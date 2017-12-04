#ifndef __KERNEL_ASSERT_H
#define __KERNEL_ASSERT_H

#include <stdint.h>
#include <stdarg.h>

namespace kernel
{
    void
    vgawrite(uint8_t x, uint8_t y, const char* s,
             uint16_t cflags = 0x4F00) noexcept;

    void
    halt() noexcept __attribute__((noreturn));

    void
    vpanic(const char* fmt, va_list ap) noexcept __attribute__((noreturn));

    __attribute__((noreturn,format(printf,1,2)))
    inline void
    panic(const char* fmt, ...) noexcept
    {
        va_list ap;
        va_start(ap,fmt);
        vpanic(fmt,ap);
        va_end(ap);
    }

     __attribute__((noreturn))
    inline void
    panic(int line = __builtin_LINE(), const char* file = __builtin_FILE())
        noexcept
    {
        panic("%s:%d",file,line);
    }

    __attribute__((format(printf,2,3)))
    inline void
    _kassert(bool expr, const char* fmt, ...)
    {
        if (!expr)
        {
            va_list ap;
            va_start(ap,fmt);
            vpanic(fmt,ap);
            va_end(ap);
        }
    }
}

#define _LINESTR(l) #l
#define LINESTR(l) _LINESTR(l)
#define FILELINESTR __FILE__ ":" LINESTR(__LINE__)
#define kassert(exp,...) _kassert((exp),FILELINESTR ":" #exp __VA_ARGS__)
#define KASSERT(exp) static_assert(exp, #exp)

#endif /* __KERNEL_ASSERT_H */
