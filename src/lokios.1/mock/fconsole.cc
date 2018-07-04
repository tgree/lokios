#include "fconsole.h"
#include "../console.h"
#include <stdio.h>

bool kernel::fconsole_suppress_output = false;

void
kernel::console::vprintf(const char* fmt, va_list ap)
{
    if (!fconsole_suppress_output)
        ::vprintf(fmt,ap);
}
