#ifndef __MODE32_ASSERT_H
#define __MODE32_ASSERT_H

#include "hdr/compiler.h"

void _abort(const char* f = __builtin_FILE(),
            int l = __builtin_LINE()) __REG_PARMS__ __NORETURN__;

inline void __REG_PARMS__ assert(bool expr, const char* f = __builtin_FILE(),
                                 int l = __builtin_LINE())
{
    if (!expr)
        _abort(f,l);
}

#endif /* __MODE32_ASSERT_H */
