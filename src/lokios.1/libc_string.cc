#include "k++/kprintf.h"
#include "kassert.h"
#include <stdio.h>

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
char* strcpy(char* dest, const char* src)
{
    char* p = dest;
    do
    {
        *p++ = *src;
    } while (*src++);
    return dest;
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

extern "C"
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

static void
sprintf_putc(void* cookie, char c)
{
    char** pp = (char**)cookie;
    char* p   = *pp;
    *p        = c;
    (*pp)++;
}

extern "C"
int sprintf(char* str, const char* fmt, ...)
{
    char* p = str;
    va_list ap;
    va_start(ap,fmt);
    kernel::kvprintf(sprintf_putc,&p,fmt,ap);
    va_end(ap);
    *p = '\0';
    return p - str;
}
