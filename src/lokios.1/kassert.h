#ifndef __KERNEL_ASSERT_H
#define __KERNEL_ASSERT_H

#include <hdr/kassert.h>
#include <hdr/compiler.h>
#include <hdr/fileline.h>
#include <stdint.h>

namespace kernel
{
    void halt() noexcept __NORETURN__;

    void panic(const char* s = "", const char* f = __builtin_FILE(),
               unsigned int l = __builtin_LINE()) noexcept __NORETURN__;

    inline void _kassert(bool expr, const char* s,
                         const char* f = __builtin_FILE(),
                         unsigned int l = __builtin_LINE())
    {
        if (!expr)
            panic(s,f,l);
    }
}
#define kassert(exp) _kassert((exp),#exp)

#endif /* __KERNEL_ASSERT_H */
