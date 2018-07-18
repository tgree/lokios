#ifndef __KERNEL_CHAR_STREAM_H
#define __KERNEL_CHAR_STREAM_H

#include "kernel/spinlock.h"
#include <stdarg.h>
#include <stddef.h>

namespace kernel
{
    class char_stream_base
    {
        void    print_field(const char* ptr, size_t digits, unsigned int flags,
                            unsigned int width, unsigned int precision);
        void    print_decimal(long long v, unsigned int flags,
                              unsigned int width, unsigned int precision);
        void    print_udecimal(unsigned long long v, unsigned int flags,
                               unsigned int width, unsigned int precision);
        void    print_octal(unsigned long long v, unsigned int flags,
                            unsigned int width, unsigned int precision);
        void    print_hex(unsigned long long v, unsigned int flags,
                          unsigned int width, unsigned int precision,
                          const char* lut);
        void    print_string(const char* s, unsigned int flags,
                             unsigned int width, unsigned int precision);

        virtual void _putc(char c) = 0;

    protected:
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

        char_stream_base();
        virtual ~char_stream_base();
    };

    template<typename Lock>
    class char_stream : public char_stream_base
    {
        Lock    lock;

    public:
        inline void jvprintf(uint64_t jiffies, const char* fmt, va_list ap)
        {
            with (lock)
            {
                locked_printf("[%3lu.%02lu] ",jiffies/100,jiffies%100);
                locked_vprintf(fmt,ap);
            }
        }

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
