#include "char_stream.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

// Flag characters.
#define PRINTF_FLAG_ALTERNATE_FORM  (1<<0)
#define PRINTF_FLAG_ZERO_PADDED     (1<<1)
#define PRINTF_FLAG_LEFT_ADJUSTED   (1<<2)
#define PRINTF_FLAG_POSITIVE_BLANK  (1<<3)
#define PRINTF_FLAG_POSITIVE_PLUS   (1<<4)
#define PRINTF_FLAG_OMIT_PRECISION  (1<<5)
#define PRINTF_FLAG_UPPERCASE       (1<<6)

// Length modifiers.
enum length_modifier_e
{
    PRINTF_LM_none,
    PRINTF_LM_h,
    PRINTF_LM_hh,
    PRINTF_LM_l,
    PRINTF_LM_ll,
    PRINTF_LM_L,
    PRINTF_LM_j,
    PRINTF_LM_z,
    PRINTF_LM_t,
};

char_stream::char_stream()
{
}

char_stream::~char_stream()
{
}

void
char_stream::print_decimal(long long v, unsigned int flags, unsigned int width,
    unsigned int precision)
{
    char buf[20];
    char* end = buf + sizeof(buf);
    char* ptr = end;
    size_t digits = 0;

    if (!v)
    {
        if (precision == 0)
            return;
        buf[19] = '0';
        ptr     = buf + 19;
        digits  = 1;
    }
    else
    {
        while (v)
        {
            *--ptr = '0' + v % 10;
            v     /= 10;
            ++digits;
        }
    }

    size_t zeroes = digits < precision ? precision - digits : 0;
    digits       += zeroes;
    size_t padcs  = digits < width ? width - digits : 0;

    if (flags & PRINTF_FLAG_LEFT_ADJUSTED)
    {
        for (size_t i=0; i<zeroes; ++i)
            _putc('0');
        while (ptr != end)
            _putc(*ptr++);
        for (size_t i=0; i<padcs; ++i)
            _putc(' ');
    }
    else
    {
        char padc = ' ';
        if (flags & PRINTF_FLAG_ZERO_PADDED)
            padc = '0';
        for (size_t i=0; i<padcs; ++i)
            _putc(padc);
        for (size_t i=0; i<zeroes; ++i)
            _putc('0');
        while (ptr != end)
            _putc(*ptr++);
    }
}

void
char_stream::print_udecimal(unsigned long long v, unsigned int flags,
    unsigned int width, unsigned int precision)
{
}

void
char_stream::print_octal(unsigned long long v, unsigned int flags,
    unsigned int width, unsigned int precision)
{
}

void
char_stream::print_hex_lc(unsigned long long v, unsigned int flags,
    unsigned int width, unsigned int precision)
{
}

void
char_stream::print_hex_uc(unsigned long long v, unsigned int flags,
    unsigned int width, unsigned int precision)
{
}

void
char_stream::print_string(const char* s, unsigned int flags,
    unsigned int width, unsigned int precision)
{
    // HACK.
    while (*s)
        _putc(*s++);
}

void
char_stream::vprintf(const char* fmt, va_list ap)
{
    char c;
    while ( (c = *fmt++) != 0)
    {
        // Handle simple chars first.
        if (c != '%')
        {
            _putc(c);
            continue;
        }

        // Okay, we have a format specifier.  Parse it.
        unsigned int flags = 0;
        while (*fmt)
        {
            const char* pos = fmt;
            switch (*fmt)
            {
                case '#': flags |= PRINTF_FLAG_ALTERNATE_FORM; ++fmt; break;
                case '0': flags |= PRINTF_FLAG_ZERO_PADDED; ++fmt;    break;
                case '-': flags |= PRINTF_FLAG_LEFT_ADJUSTED; ++fmt;  break;
                case ' ': flags |= PRINTF_FLAG_POSITIVE_BLANK; ++fmt; break;
                case '+': flags |= PRINTF_FLAG_POSITIVE_PLUS; ++fmt;  break;
            }
            if (pos == fmt)
                break;
        }

        // Now we may have a field width.
        unsigned int width = 0;
        while (*fmt)
        {
            const char* pos = fmt;
            switch (*fmt)
            {
                case '*':
                    // We don't support dynamic field widths right now.
                    abort();
                break;

                case '0'...'9':
                    width *= 10;
                    width += *fmt - '0';
                    ++fmt;
                break;
            }
            if (pos == fmt)
                break;
        }

        // Now we may have a precision.
        unsigned int precision = 0;
        if (*fmt == '.')
        {
            ++fmt;
            if (*fmt && *fmt == '-')
            {
                flags |= PRINTF_FLAG_OMIT_PRECISION;
                ++fmt;
            }
            while (*fmt)
            {
                const char* pos = fmt;
                switch (*fmt)
                {
                    case '0'...'9':
                        precision *= 10;
                        precision += *fmt - '0';
                        ++fmt;
                    break;
                }
                if (pos == fmt)
                    break;
            }
        }
        else
            flags |= PRINTF_FLAG_OMIT_PRECISION;

        // Now we may have a length modifier.
        length_modifier_e length_modifier = PRINTF_LM_none;
        switch (*fmt)
        {
            case 'h':
                ++fmt;
                if (*fmt == 'h')
                {
                    length_modifier = PRINTF_LM_hh;
                    ++fmt;
                }
                else
                    length_modifier = PRINTF_LM_h;
            break;

            case 'l':
                ++fmt;
                if (*fmt == 'l')
                {
                    length_modifier = PRINTF_LM_ll;
                    ++fmt;
                }
                else
                    length_modifier = PRINTF_LM_l;
            break;

            case 'L': length_modifier = PRINTF_LM_L; ++fmt; break;
            case 'j': length_modifier = PRINTF_LM_j; ++fmt; break;
            case 'z': length_modifier = PRINTF_LM_z; ++fmt; break;
            case 't': length_modifier = PRINTF_LM_t; ++fmt; break;
        }

        // Finally we have a conversion specifier.
        switch (*fmt)
        {
            case 'd':
            case 'i':
            {
                long long v;
                switch (length_modifier)
                {
                    case PRINTF_LM_none: v = va_arg(ap,int);         break;
                    case PRINTF_LM_h:    v = va_arg(ap,int);         break;
                    case PRINTF_LM_hh:   v = va_arg(ap,int);         break;
                    case PRINTF_LM_l:    v = va_arg(ap,long);        break;
                    case PRINTF_LM_ll:   v = va_arg(ap,long long);   break;
                    case PRINTF_LM_j:    v = va_arg(ap,intmax_t);    break;
                    case PRINTF_LM_z:    v = va_arg(ap,ssize_t);     break;
                    case PRINTF_LM_t:    v = va_arg(ap,ptrdiff_t);   break;

                    case PRINTF_LM_L:
                        // We don't support long double output.
                        abort();
                    break;

                    default:
                        // Bizarre.
                        return;
                    break;
                }
                print_decimal(v,flags,width,precision);
                ++fmt;
            }
            break;

            case 'o':
            case 'u':
            case 'x':
            case 'X':
            {
                unsigned long long v;
                switch (length_modifier)
                {
                    case PRINTF_LM_none: v = va_arg(ap,unsigned int);     break;
                    case PRINTF_LM_h:  v = va_arg(ap,unsigned int);       break;
                    case PRINTF_LM_hh: v = va_arg(ap,unsigned int);       break;
                    case PRINTF_LM_l:  v = va_arg(ap,unsigned long);      break;
                    case PRINTF_LM_ll: v = va_arg(ap,unsigned long long); break;
                    case PRINTF_LM_j:  v = va_arg(ap,uintmax_t);          break;
                    case PRINTF_LM_z:  v = va_arg(ap,size_t);             break;
                    case PRINTF_LM_t:  v = va_arg(ap,ptrdiff_t);          break;

                    case PRINTF_LM_L:
                        // We don't support long double output.
                        abort();
                    break;

                    default:
                        // Bizarre.
                        return;
                    break;
                }
                switch (*fmt)
                {
                    case 'o': print_octal(v,flags,width,precision);    break;
                    case 'u': print_udecimal(v,flags,width,precision); break;
                    case 'x': print_hex_lc(v,flags,width,precision);   break;
                    case 'X': print_hex_uc(v,flags,width,precision);   break;
                }
                ++fmt;
            }
            break;

            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
                // We don't support double output.
                abort();
            break;

            case 'c':
                // We don't support wide chars.
                if (length_modifier != PRINTF_LM_none)
                    abort();
                _putc(*fmt++);
            break;

            case 's':
            {
                // We don't support wide chars.
                if (length_modifier != PRINTF_LM_none)
                    abort();
                const char* s = va_arg(ap,const char*);
                print_string(s,flags,width,precision);
                ++fmt;
            }
            break;

            case 'p':
            {
                void* p = va_arg(ap,void*);
                print_hex_lc((unsigned long long)p,
                             flags | PRINTF_FLAG_ALTERNATE_FORM,width,
                             precision);
                ++fmt;
            }
            break;

            case 'n':
                // We don't support storing back into the arguments.
                abort();
            break;

            case 'm':
                // We don't support the glibc strerror extension.
                abort();
            break;

            case '%':
                _putc('%');
                ++fmt;
            break;

            default:
                // Something unrecognized.
                return;
            break;
        }
    }
}