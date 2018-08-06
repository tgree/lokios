#include <string.h>

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

void*
memcpy(void* _dest, const void* _src, size_t n)
{
    auto* d = (char*)_dest;
    auto* s = (const char*)_src;
    while (n--)
        *d++ = *s++;
    return _dest;
}

extern "C"
void* memset(void* _s, int c, size_t n)
{
    char* s = (char*)_s;
    while (n--)
        *s++ = c;
    return _s;
}
