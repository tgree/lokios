#ifndef __MODE32_CONSOLE_H
#define __MODE32_CONSOLE_H

#include "hdr/compiler.h"
#include <stdarg.h>

namespace console
{
    void vprintf(const char* fmt, va_list ap);
    inline void __PRINTF__(1,2) printf(const char* fmt, ...)
    {
        va_list ap;
        va_start(ap,fmt);
        vprintf(fmt,ap);
        va_end(ap);
    }

    void init();
}

#endif /* __MODE32_CONSOLE_H */
