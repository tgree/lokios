#ifndef __KERNEL_CHAR_STREAM_H
#define __KERNEL_CHAR_STREAM_H

#include "kern/spinlock.h"
#include <stdarg.h>
#include <stddef.h>

namespace kernel
{
    class char_stream_base
    {
        virtual void _putc(char c) = 0;

        static void _putc_bounce(void* cookie, char c);

    public:
        void    locked_vprintf(const char* fmt, va_list ap);
        void    locked_hexdump(const void* addr, size_t len,
                               unsigned long base);
        inline void locked_printf(const char* fmt, ...)
            __attribute__((format(printf,2,3)))
        {
            va_list ap;
            va_start(ap,fmt);
            locked_vprintf(fmt,ap);
            va_end(ap);
        }

    protected:
        char_stream_base() {}
        virtual ~char_stream_base() {}
    };

    template<typename Lock>
    struct char_stream : public char_stream_base
    {
        Lock    lock;

        inline void vprintf(const char* fmt, va_list ap)
        {
            with (lock)
            {
                locked_vprintf(fmt,ap);
            }
        }

        inline void printf(const char* fmt, ...)
            __attribute__((format(printf,2,3)))
        {
            va_list ap;
            va_start(ap,fmt);
            vprintf(fmt,ap);
            va_end(ap);
        }

        inline void hexdump(const void* addr, size_t len, unsigned long base)
        {
            with (lock)
            {
                locked_hexdump(addr,len,base);
            }
        }
    };
}

#endif /* __KERNEL_CHAR_STREAM_H */
