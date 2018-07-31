#ifndef __KERNEL_ASSERT_H
#define __KERNEL_ASSERT_H

#include <hdr/kassert.h>
#include <hdr/compiler.h>
#include <hdr/fileline.h>
#include <stdint.h>

namespace kernel
{
    void halt() noexcept __NORETURN__;

    void panic(const char* s = "") noexcept __NORETURN__;

    inline void _kassert(bool expr, const char* s)
    {
        if (!expr)
            panic(s);
    }
}

#define kassert(exp) _kassert((exp),FILELINESTR ":" #exp)

#endif /* __KERNEL_ASSERT_H */
