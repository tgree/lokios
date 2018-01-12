#include "libc.h"
#include "console.h"
#include "kassert.h"
#include "mm/sbrk.h"
#include "mm/page.h"
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

FILE* stderr;

extern "C"
void* malloc(size_t n) noexcept
{
    if (n <= PAGE_SIZE)
        return kernel::page_alloc();
    return kernel::sbrk(n);
}

extern "C"
void* realloc(void* ptr, size_t size)
{
    // Check if this is a malloc- or free-equivalent.
    if (!ptr)
        return malloc(size);
    if (ptr && !size)
    {
        free(ptr);
        return NULL;
    }

    // If this was allocated on a page, well we have 4K of space already...
    kernel::kassert(ptr >= kernel::sbrk(0));
    kernel::kassert(size <= PAGE_SIZE);
    return ptr;
}

extern "C"
void free(void* p) noexcept
{
    kernel::kassert(p >= kernel::sbrk(0));
    kernel::page_free(p);
}

extern "C"
size_t strlen(const char* s)
{
    const char* e = s;
    while (*e)
        ++e;

    return e - s;
}

extern "C"
size_t strnlen(const char* s, size_t maxlen)
{
    const char* e = s;
    while (maxlen-- && *e)
        ++e;

    return e - s;
}

extern "C"
void* memcpy(void* dest, const void* src, size_t n)
{
    const char* s = (const char*)src;
    char* d = (char*)dest;
    while (n--)
        *d++ = *s++;
    return dest;
}

extern "C"
int memcmp(const void* _s1, const void* _s2, size_t n)
{
    kernel::console::printf("memcmp\n");
    const char* s1 = (const char*)_s1;
    const char* s2 = (const char*)_s2;
    while (n)
    {
        if (*s1 < *s2)
            return -1;
        if (*s1 > *s2)
            return 1;
        ++s1;
        ++s2;
    }
    return 0;
}

extern "C"
void* memset(void* _s, int c, size_t n)
{
    char* s = (char*)_s;
    while (n--)
        *s++ = c;
    return _s;
}

extern "C"
int strcmp(const char* s1, const char* s2)
{
    while (*s1 && *s2)
    {
        if (*s1 < *s2)
            return -1;
        if (*s1 > *s2)
            return 1;
        ++s1;
        ++s2;
    }
    if (!*s1 && !*s2)
        return 0;
    if (!*s1)
        return -1;
    return 1;
}

extern "C"
int strncmp(const char* s1, const char* s2, size_t n)
{
    while (n && *s1 && *s2)
    {
        if (*s1 < *s2)
            return -1;
        if (*s1 > *s2)
            return 1;
        ++s1;
        ++s2;
        --n;
    }
    if (!n)
        return 0;
    if (!*s1 && !*s2)
        return 0;
    if (!*s1)
        return -1;
    return 1;
}

extern "C"
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    kernel::kassert(stream == stderr);
    const char* p = (const char*)ptr;
    for (size_t i=0; i<nmemb; ++i)
        for (size_t j=0; j<size; ++j)
            kernel::console::_putc(*p++);
    return nmemb;
}

extern "C"
int fputc(int c, FILE* stream)
{
    kernel::kassert(stream == stderr);
    kernel::console::_putc(c);
    return (unsigned char)c;
}

extern "C"
int fputs(const char* s, FILE* stream)
{
    kernel::kassert(stream == stderr);
    kernel::console::printf("%s",s);
    return 0;
}

extern "C"
int fprintf(FILE* stream, const char* fmt, ...)
{
    // TODO: char_stream::printf() doesn't return the number of characters
    // printed, so we can't return it here either.  I doubt libsupc++ relies on
    // that value, though...
    kernel::kassert(stream == stderr);
    va_list ap;
    va_start(ap,fmt);
    kernel::console::vprintf(fmt,ap);
    va_end(ap);
    return 0;
}

extern "C"
ssize_t write(int fd, const void* buf, size_t count)
{
    kernel::kassert(fd == STDERR_FILENO);
    const char* p = (const char*)buf;
    while (count--)
        kernel::console::_putc(*p++);
    return count;
}

extern "C"
int dl_iterate_phdr(
	 int (*callback) (struct dl_phdr_info* info,
			  size_t size, void* data),
	 void* data)
{
    // This gets called by the exception-handling code to try and start finding
    // exception frame entries for dynamically-loaded objects.  In our case it
    // it will fire if we try to throw off the top of the stack; returning -1
    // is probably the best we can do.
    return -1;
}
