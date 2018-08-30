#include "console.h"
#include "time.h"

static kernel::klist_leaks<kernel::kconsole> consoles;

void
kernel::console::register_console(kconsole* kc)
{
    consoles.push_back(&kc->kc_link);
}

void
kernel::console::vprintf(const char* fmt, va_list ap)
{
    uint64_t jiffies = get_jiffies();

    for (auto& kc : klist_elems(consoles,kc_link))
    {
        va_list ap2;
        va_copy(ap2,ap);
        with (kc.lock)
        {
            kc.locked_printf("[%3lu.%02lu] ",jiffies/100,jiffies%100);
            kc.locked_vprintf(fmt,ap2);
        }
        va_end(ap2);
    }
}

void
kernel::console::v2printf(const char* fmt1, va_list ap1, const char* fmt2,
    va_list ap2)
{
    uint64_t jiffies = get_jiffies();

    for (auto& kc : klist_elems(consoles,kc_link))
    {
        va_list apc1;
        va_copy(apc1,ap1);
        va_list apc2;
        va_copy(apc2,ap2);
        with (kc.lock)
        {
            kc.locked_printf("[%3lu.%02lu] ",jiffies/100,jiffies%100);
            kc.locked_vprintf(fmt1,apc1);
            kc.locked_vprintf(fmt2,apc2);
        }
        va_end(apc2);
        va_end(apc1);
    }
}

void
kernel::console::_putc(char c)
{
    for (auto& kc : klist_elems(consoles,kc_link))
        kc.printf("%c",c);
}

void
kernel::console::hexdump(const void* addr, size_t len, unsigned long base)
{
    for (auto& kc : klist_elems(consoles,kc_link))
        kc.hexdump(addr,len,base);
}
