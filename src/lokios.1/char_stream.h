#ifndef __KERNEL_CHAR_STREAM_H
#define __KERNEL_CHAR_STREAM_H

#include <stdarg.h>
#include <stddef.h>

class char_stream
{
    void    print_field(const char* ptr, size_t digits, unsigned int flags,
                        unsigned int width, unsigned int precision);
    void    print_decimal(long long v, unsigned int flags, unsigned int width,
                          unsigned int precision);
    void    print_udecimal(unsigned long long v, unsigned int flags,
                           unsigned int width, unsigned int precision);
    void    print_octal(unsigned long long v, unsigned int flags,
                        unsigned int width, unsigned int precision);
    void    print_hex(unsigned long long v, unsigned int flags,
                      unsigned int width, unsigned int precision,
                      const char* lut);
    void    print_string(const char* s, unsigned int flags,
                         unsigned int width, unsigned int precision);

public:
    virtual void _putc(char c) = 0;

            void vprintf(const char* fmt, va_list ap);
    inline  void printf(const char* fmt, ...)
        __attribute__((format(printf,2,3)))
    {
        va_list ap;
        va_start(ap,fmt);
        vprintf(fmt,ap);
        va_end(ap);
    }

    char_stream();
    virtual ~char_stream();
};

#endif /* __KERNEL_CHAR_STREAM_H */
