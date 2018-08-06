#ifndef __KERNEL_KPRINTF_H
#define __KERNEL_KPRINTF_H

#include "hdr/compiler.h"
#include <stdarg.h>

namespace kernel
{
    typedef void (*putc_func)(void* cookie, char c);
    void kvprintf(putc_func p, void* cookie, const char* fmt, va_list ap);

    inline void __PRINTF__(3,4) kprintf(putc_func p, void* cookie,
                                        const char* fmt, ...)
    {
        va_list ap;
        va_start(ap,fmt);
        kvprintf(p,cookie,fmt,ap);
        va_end(ap);
    }
}

#endif /* __KERNEL_KPRINTF_H */
