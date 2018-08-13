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
