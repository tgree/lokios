#include "tmock.h"
#include <string.h>
#include <stdio.h>

void
tmock::vabort(const char* fmt, va_list ap)
{
    if (!(tmock::internal::mode_flags & TMOCK_MODE_FLAG_SILENT))
        vprintf(fmt,ap);
    ::abort();
}

void
tmock::assert_equiv(const char* s, const char* expected)
{
    if (strcmp(s,expected))
    {
        if (!(tmock::internal::mode_flags & TMOCK_MODE_FLAG_SILENT))
        {
            printf("Expected: '%s'\n",expected);
            printf("     Got: '%s'\n",s);
        }
        exit(-1);
    }
}

void
tmock::assert_equiv(uint16_t v, uint16_t expected)
{
    if (v != expected)
    {
        if (!(tmock::internal::mode_flags & TMOCK_MODE_FLAG_SILENT))
        {
            printf("Expected: 0x%04X\n",expected);
            printf("     Got: 0x%04X\n",v);
        }
        exit(-1);
    }
}
