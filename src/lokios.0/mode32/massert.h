#ifndef __MODE32_ASSERT_H
#define __MODE32_ASSERT_H

#include "hdr/compiler.h"

extern "C"
{
    void m32_abort(const char* f = __builtin_FILE(),
                   int l = __builtin_LINE()) __REG_PARMS__ __NORETURN__;

    inline void __REG_PARMS__ m32_assert(bool expr,
                                         const char* f = __builtin_FILE(),
                                         int l = __builtin_LINE())
    {
        if (!expr)
            m32_abort(f,l);
    }
}

#endif /* __MODE32_ASSERT_H */
