#include "fconsole.h"
#include "../console.h"
#include <stdio.h>

bool kernel::fconsole_suppress_output = false;

void
kernel::console::vprintf(const char* fmt, va_list ap)
{
    if (fconsole_suppress_output)
        return;

    ::vprintf(fmt,ap);
}

void
kernel::console::v2printf(const char* fmt1, va_list ap1, const char* fmt2,
    va_list ap2)
{
    if (fconsole_suppress_output)
        return;

    ::vprintf(fmt1,ap1);
    ::vprintf(fmt2,ap2);
}

void
kernel::console::_putc(char c)
{
    kernel::console::printf("%c",c);
}

static const char isprint_table[] =
    "................................ !\"#$%&'()*+,-./0123456789:;<=>?"
    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~."
    "................................................................"
    "................................................................";

void
kernel::console::hexdump(const void* addr, size_t len, unsigned long base)
{
    if (fconsole_suppress_output)
        return;

    const unsigned char* p = (const unsigned char*)addr;
    while (len >= 16)
    {
        printf("%08lX: %02X %02X %02X %02X  %02X %02X %02X %02X  "
               "%02X %02X %02X %02X  %02X %02X %02X %02X "
               "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
               base,p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],p[10],
               p[11],p[12],p[13],p[14],p[15],
               isprint_table[p[0]],
               isprint_table[p[1]],
               isprint_table[p[2]],
               isprint_table[p[3]],
               isprint_table[p[4]],
               isprint_table[p[5]],
               isprint_table[p[6]],
               isprint_table[p[7]],
               isprint_table[p[8]],
               isprint_table[p[9]],
               isprint_table[p[10]],
               isprint_table[p[11]],
               isprint_table[p[12]],
               isprint_table[p[13]],
               isprint_table[p[14]],
               isprint_table[p[15]]
               );
        base += 16;
        p    += 16;
        len  -= 16;
    }
    if (!len)
        return;

    printf("%08lX:",base);
    for (size_t i=0; i<16; ++i)
    {
        if (i < len)
            printf(" %02X",p[i]);
        else
            printf("   ");
        if (i % 4 == 3)
            printf(" ");
    }
    for (size_t i=0; i<len; ++i)
        printf("%c",isprint_table[p[i]]);
    printf("\n");
}
