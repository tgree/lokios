#include "k++/kprintf.h"
#include "k++/kmath.h"
#include "kern/libc.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

using kernel::putc_func;

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

static const char* hex_uc_lut = "0123456789ABCDEF";
static const char* hex_lc_lut = "0123456789abcdef";

static void
print_field(putc_func _putc, void* cookie, const char* ptr, size_t digits,
    unsigned int flags, unsigned int width, unsigned int precision)
{
    size_t zeroes = digits < precision ? precision - digits : 0;
    digits       += zeroes;
    size_t padcs  = digits < width ? width - digits : 0;

    if (flags & PRINTF_FLAG_LEFT_ADJUSTED)
    {
        if (flags & PRINTF_FLAG_POSITIVE_PLUS)
            _putc(cookie,'+');
        else if (flags & PRINTF_FLAG_POSITIVE_BLANK)
            _putc(cookie,' ');
        for (size_t i=0; i<zeroes; ++i)
            _putc(cookie,'0');
        while (digits--)
            _putc(cookie,*ptr++);
        for (size_t i=0; i<padcs; ++i)
            _putc(cookie,' ');
    }
    else
    {
        char padc = ' ';
        if (flags & PRINTF_FLAG_ZERO_PADDED)
            padc = '0';
        for (size_t i=0; i<padcs; ++i)
            _putc(cookie,padc);
        if (flags & PRINTF_FLAG_POSITIVE_PLUS)
            _putc(cookie,'+');
        else if (flags & PRINTF_FLAG_POSITIVE_BLANK)
            _putc(cookie,' ');
        for (size_t i=0; i<zeroes; ++i)
            _putc(cookie,'0');
        while (digits--)
            _putc(cookie,*ptr++);
    }
}

static void
print_udecimal(putc_func _putc, void* cookie, unsigned long long v,
    unsigned int flags, unsigned int width, unsigned int precision)
{
    char buf[20];
    char* ptr = buf + sizeof(buf);
    size_t digits;

    if (!v)
    {
        if (precision == 0 && !(flags & PRINTF_FLAG_OMIT_PRECISION))
            return;
        buf[19] = '0';
        ptr     = buf + 19;
        digits  = 1;
    }
    else
    {
        digits = 0;
        while (v)
        {
            *--ptr = '0' + v % 10;
            v     /= 10;
            ++digits;
        }
    }

    if (flags & (PRINTF_FLAG_POSITIVE_PLUS | PRINTF_FLAG_POSITIVE_BLANK))
    {
        if (width)
            --width;
    }

    print_field(_putc,cookie,ptr,digits,flags,width,precision);
}

static void
print_decimal(putc_func _putc, void* cookie, long long v, unsigned int flags,
    unsigned int width, unsigned int precision)
{
    if (v < 0)
    {
        _putc(cookie,'-');
        v = -v;
        if (width)
            --width;
        flags &= ~(PRINTF_FLAG_POSITIVE_PLUS | PRINTF_FLAG_POSITIVE_BLANK);
    }
    print_udecimal(_putc,cookie,v,flags,width,precision);
}

void
print_octal(putc_func _putc, void* cookie, unsigned long long v,
    unsigned int flags, unsigned int width, unsigned int precision)
{
    char buf[22];
    char* ptr = buf + sizeof(buf);
    size_t digits;

    if (!v)
    {
        if (precision == 0 && !(flags & PRINTF_FLAG_OMIT_PRECISION))
            return;
        buf[21] = '0';
        ptr     = buf + 21;
        digits  = 1;
    }
    else
    {
        digits = 0;
        while (v)
        {
            *--ptr = '0' + v % 8;
            v     /= 8;
            ++digits;
        }
    }

    if (flags & (PRINTF_FLAG_POSITIVE_PLUS | PRINTF_FLAG_POSITIVE_BLANK))
    {
        if (width)
            --width;
    }

    print_field(_putc,cookie,ptr,digits,flags,width,precision);
}

static void
print_hex(putc_func _putc, void* cookie, unsigned long long v,
    unsigned int flags, unsigned int width, unsigned int precision,
    const char* lut)
{
    char buf[19];
    char* ptr = buf + sizeof(buf);
    size_t digits;
    
    if (!v)
    {
        if (precision == 0 && !(flags & PRINTF_FLAG_OMIT_PRECISION))
            return;
        buf[18] = '0';
        ptr     = buf + 18;
        digits  = 1;
    }
    else
    {
        digits = 0;
        while (v)
        {
            *--ptr = lut[v & 0x0F];
            v    >>= 4;
            ++digits;
        }
    }

    print_field(_putc,cookie,ptr,digits,flags,width,precision);
}

static void
print_string(putc_func _putc, void* cookie, const char* s, unsigned int flags,
    unsigned int width, unsigned int precision)
{
    size_t digits;
    if (flags & PRINTF_FLAG_OMIT_PRECISION)
        digits = strlen(s);
    else
        digits = MIN(strnlen(s,precision),precision);
    flags &= ~PRINTF_FLAG_ZERO_PADDED;

    print_field(_putc,cookie,s,digits,flags,width,0);
}

void
kernel::kvprintf(putc_func _putc, void* cookie, const char* fmt, va_list ap)
{
    char c;
    while ( (c = *fmt++) != 0)
    {
        // Handle simple chars first.
        if (c != '%')
        {
            _putc(cookie,c);
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
                print_decimal(_putc,cookie,v,flags,width,precision);
                ++fmt;
            }
            break;

            case 'o':
            case 'u':
            case 'x':
            case 'X':
            {
                flags &= ~PRINTF_FLAG_POSITIVE_PLUS;
                flags &= ~PRINTF_FLAG_POSITIVE_BLANK;

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
                    case 'o':
                        print_octal(_putc,cookie,v,flags,width,precision);
                    break;
                    case 'u':
                        print_udecimal(_putc,cookie,v,flags,width,precision);
                    break;
                    case 'x':
                        print_hex(_putc,cookie,v,flags,width,precision,
                                  hex_lc_lut);
                    break;
                    case 'X':
                        print_hex(_putc,cookie,v,flags,width,precision,
                                  hex_uc_lut);
                    break;
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
            {
                // We don't support wide chars.
                if (length_modifier != PRINTF_LM_none)
                    abort();
                int c = va_arg(ap,int);
                _putc(cookie,c);
                ++fmt;
            }
            break;

            case 's':
            {
                // We don't support wide chars.
                if (length_modifier != PRINTF_LM_none)
                    abort();
                const char* s = va_arg(ap,const char*);
                print_string(_putc,cookie,s,flags,width,precision);
                ++fmt;
            }
            break;

            case 'p':
            {
                void* p = va_arg(ap,void*);
                _putc(cookie,'0');
                _putc(cookie,'x');
                print_hex(_putc,cookie,(unsigned long long)p,
                          flags | PRINTF_FLAG_ALTERNATE_FORM,width,
                          precision,hex_lc_lut);
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
                _putc(cookie,'%');
                ++fmt;
            break;

            default:
                // Something unrecognized.
                return;
            break;
        }
    }
}
