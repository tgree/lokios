#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include "console.h"
#include "assert.h"

FILE* stderr;
static char* FREEMEM = (char*)0x00300000;

extern "C"
void* malloc(size_t n)
{
    char* rv = FREEMEM;
    n        = ((n + 7)/8)*8;
    FREEMEM += n;
    vga.printf("malloc(%zd) = %ld\n",n,(uintptr_t)rv);
    return rv;
}

extern "C"
void* realloc(void* ptr, size_t size)
{
    aborts("REALLOC");
}

extern "C"
void free(void* p)
{
}

extern "C"
size_t strlen(const char* s)
{
    vga.printf("strlen(\"%s\") = ",s);
    const char* e = s;
    while (*e)
        ++e;

    size_t rv = (e - s);
    vga.printf("%zd\n",rv);
    return rv;
}

extern "C"
void* memcpy(void* dest, const void* src, size_t n)
{
    vga.printf("memcpy\n");
    const char* s = (const char*)src;
    char* d = (char*)dest;
    while (n--)
        *d++ = *s++;
    return dest;
}

extern "C"
int memcmp(const void* _s1, const void* _s2, size_t n)
{
    vga.printf("memcmp\n");
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
int strcmp(const char* s1, const char* s2)
{
    vga.printf("strcmp\n");
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
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return 0;
}

extern "C"
int fputc(int c, FILE *stream)
{
    return 0;
}

extern "C"
int fputs(const char* s, FILE* stream)
{
    return 0;
}

extern "C"
int fprintf(FILE *stream, const char* fmt, ...)
{
    return 0;
}

extern "C"
ssize_t write(int fd, const void *buf, size_t count)
{
    return 0;
}

extern "C"
int dl_iterate_phdr(
	 int (*callback) (struct dl_phdr_info *info,
			  size_t size, void *data),
	 void *data)
{
    // This gets called by the exception-handling code to try and start finding
    // exception frame entries for dynamically-loaded objects.  In our case it
    // it will fire if we try to throw off the top of the stack; returning -1
    // is probably the best we can do.
    vga.printf("dl_iterate_phdr\n");
    return -1;
}
