#ifndef __KERNEL_ASSERT_H
#define __KERNEL_ASSERT_H

#include <hdr/fileline.h>
#include <stdint.h>

namespace kernel
{
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

#define kassert(exp) _kassert((exp),FILELINESTR ":" #exp)
#define KASSERT(exp) static_assert(exp, #exp)

#endif /* __KERNEL_ASSERT_H */
