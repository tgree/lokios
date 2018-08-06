#include "k++/char_stream.h"
#include "kprintf.h"

kernel::char_stream_base::char_stream_base()
{
}

kernel::char_stream_base::~char_stream_base()
{
}

void
kernel::char_stream_base::_putc_bounce(void* cookie, char c)
{
    ((kernel::char_stream_base*)cookie)->_putc(c);
}

void
kernel::char_stream_base::locked_vprintf(const char* fmt, va_list ap)
{
    kernel::kvprintf(_putc_bounce,this,fmt,ap);
}

static const char isprint_table[] =
    "................................ !\"#$%&'()*+,-./0123456789:;<=>?"
    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~."
    "................................................................"
    "................................................................";

void
kernel::char_stream_base::locked_hexdump(const void* addr, size_t len,
    unsigned long base)
{
    const unsigned char* p = (const unsigned char*)addr;
    while (len >= 16)
    {
        locked_printf("%08lX: %02X %02X %02X %02X  %02X %02X %02X %02X  "
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

    locked_printf("%08lX:",base);
    for (size_t i=0; i<16; ++i)
    {
        if (i < len)
            locked_printf(" %02X",p[i]);
        else
            locked_printf("   ");
        if (i % 4 == 3)
            locked_printf(" ");
    }
    for (size_t i=0; i<len; ++i)
        locked_printf("%c",isprint_table[p[i]]);
    locked_printf("\n");
}
