#ifndef __KERNEL_ASSERT_H
#define __KERNEL_ASSERT_H

#include <stdint.h>

namespace kernel
{
    void
    vgawrite(uint8_t x, uint8_t y, const char* s,
             uint16_t cflags = 0x4F00) noexcept;

    void
    halt() noexcept __attribute__((noreturn));

    void
    panic(const char* s = "") noexcept __attribute__((noreturn));

    inline void
    _kassert(bool expr, const char* s)
    {
        if (!expr)
            panic(s);
    }
}

#define _LINESTR(l) #l
#define LINESTR(l) _LINESTR(l)
#define FILELINESTR __FILE__ ":" LINESTR(__LINE__)
#define kassert(exp) _kassert((exp),FILELINESTR ":" #exp)
#define KASSERT(exp) static_assert(exp, #exp)

#endif /* __KERNEL_ASSERT_H */
