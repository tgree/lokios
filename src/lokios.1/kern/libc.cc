#include "libc.h"
#include "console.h"
#include "kassert.h"
#include "mm/page.h"
#include "mm/buddy_allocator.h"
#include "mm/mm.h"
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

FILE* stderr;

#define MALLOC_ALLOC_SIG    char_code("MALA")
#define MALLOC_FREE_SIG     char_code("MALF")
struct malloc_chunk
{
    uint32_t    sig;
    uint32_t    order;
    size_t      len;
    char        data[];
};
KASSERT(sizeof(malloc_chunk) == 16);
KASSERT(offsetof(malloc_chunk,data) % 16 == 0);

extern "C"
void* malloc(size_t n) noexcept
{
    n           += sizeof(malloc_chunk);
    size_t order = kernel::buddy_order_for_len(n);
    auto* mc     = (malloc_chunk*)kernel::buddy_alloc(order);
    mc->sig      = MALLOC_ALLOC_SIG;
    mc->order    = order;
    mc->len      = n;
    return mc->data;
}

extern "C"
void* realloc(void* ptr, size_t size)
{
    // If ptr is NULL, we just malloc().
    if (!ptr)
        return malloc(size);

    // If size is 0, we just free().
    if (ptr && !size)
    {
        free(ptr);
        return NULL;
    }

    // Find the chunk header.
    auto* mc = container_of(ptr,malloc_chunk,data);
    kernel::kassert(mc->sig == MALLOC_ALLOC_SIG);

    // If the new order is the same as the old, we're done.  Otherwise we need
    // to grow or shrink the block.
    size_t new_order = kernel::buddy_order_for_len(size);
    if (new_order == mc->order)
    {
        mc->len = size;
        return mc->data;
    }

    // Allocate a new pointer.
    void* p = malloc(size);
    memcpy(p,ptr,MIN(size,mc->len));
    free(ptr);
    return p;
}

extern "C"
void free(void* p) noexcept
{
    if (!p)
        return;

    auto* mc = container_of(p,malloc_chunk,data);
    kernel::kassert(mc->sig == MALLOC_ALLOC_SIG);
    mc->sig = MALLOC_FREE_SIG;
    kernel::buddy_free(mc,mc->order);
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
char* strcpy(char* dst, const char* src)
{
    char* d = dst;
    while (*src)
        *dst++ = *src++;
    *dst = '\0';
    return d;
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
void* memmove(void* dest, const void* src, size_t n)
{
    if (dest <= src)
        return memcpy(dest,src,n);

    const char* s = (const char*)src + n;
    char* d       = (char*)dest + n;
    while (n--)
        *--d = *--s;
    kernel::kassert(dest == d);
    return dest;
}

extern "C"
int memcmp(const void* _s1, const void* _s2, size_t n)
{
    const char* s1 = (const char*)_s1;
    const char* s2 = (const char*)_s2;
    while (n--)
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

const char* strstr(const char* haystack, const char* needle)
{
    size_t n = strlen(needle);
    while (*haystack)
    {
        if (!strncmp(haystack,needle,n))
            return haystack;
        ++haystack;
    }
    return NULL;
}

void* memmem(const void* haystack, size_t haystacklen, const void* needle,
    size_t needlelen)
{
    if (needlelen > haystacklen)
        return NULL;

    const char* h = (const char*)haystack;
    const char* n = (const char*)needle;
    const char* end = h + haystacklen - needlelen + 1;
    while (h != end)
    {
        if (!memcmp(h,n,needlelen))
            return (void*)h;
        ++h;
    }
    return NULL;
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
    while (*s)
        kernel::console::_putc(*s++);
    return 0;
}

extern "C"
int fprintf(FILE* stream, const char* fmt, ...)
{
    // char_stream::printf() doesn't return the number of characters printed,
    // so we can't return it here either.  I doubt libsupc++ relies on that
    // value, though...
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
