#ifndef __KERNEL_CONSOLE_H
#define __KERNEL_CONSOLE_H

#include "k++/char_stream.h"
#include "k++/klist.h"
#include "hdr/compiler.h"
#include <stdint.h>

namespace kernel
{
    struct kconsole : public char_stream<spinlock>
    {
        klink kc_link;
    };
}

namespace kernel::console
{
    void vprintf(const char* fmt, va_list ap);
    inline void __PRINTF__(1,2) printf(const char* fmt, ...)
    {
        va_list ap;
        va_start(ap,fmt);
        kernel::console::vprintf(fmt,ap);
        va_end(ap);
    }

    void v2printf(const char* fmt1, va_list ap1, const char* fmt2, va_list ap2);

    void _putc(char c);

    void hexdump(const void* addr, size_t len, unsigned long base);

    void register_console(kconsole* kc);
}

#endif /* __KERNEL_CONSOLE_H */
