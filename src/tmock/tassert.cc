#include "tmock.h"
#include <string.h>
#include <stdio.h>

void
tmock::abort(const char* s)
{
    if (!(tmock::internal::mode_flags & TMOCK_MODE_FLAG_SILENT))
        printf("%s\n",s);
    ::abort();
}

void
tmock::assert_equiv(const char* s, const char* expected, const char* file,
    size_t line)
{
    if (strcmp(s,expected))
    {
        if (!(tmock::internal::mode_flags & TMOCK_MODE_FLAG_SILENT))
        {
            printf("%s:%zu:\n",file,line);
            printf(" Expected: '%s'\n",expected);
            printf("      Got: '%s'\n",s);
        }
        exit(-1);
    }
}

void
tmock::assert_equiv(uint16_t v, uint16_t expected, const char* file,
    size_t line)
{
    if (v != expected)
    {
        if (!(tmock::internal::mode_flags & TMOCK_MODE_FLAG_SILENT))
        {
            printf("%s:%zu:\n",file,line);
            printf(" Expected: 0x%04X\n",expected);
            printf("      Got: 0x%04X\n",v);
        }
        exit(-1);
    }
}

void
tmock::assert_equiv(uint64_t v, uint64_t expected, const char* file,
    size_t line)
{
    if (v != expected)
    {
        if (!(tmock::internal::mode_flags & TMOCK_MODE_FLAG_SILENT))
        {
            printf("%s:%zu:\n",file,line);
            printf(" Expected: 0x%016lX\n",expected);
            printf("      Got: 0x%016lX\n",v);
        }
        exit(-1);
    }
}

void
tmock::assert_equiv(int32_t v, int32_t expected, const char* file,
    size_t line)
{
    if (v != expected)
    {
        if (!(tmock::internal::mode_flags & TMOCK_MODE_FLAG_SILENT))
        {
            printf("%s:%zu:\n",file,line);
            printf(" Expected: %u\n",expected);
            printf("      Got: %u\n",v);
        }
        exit(-1);
    }
}

void
tmock::assert_equiv(int64_t v, int64_t expected, const char* file,
    size_t line)
{
    if (v != expected)
    {
        if (!(tmock::internal::mode_flags & TMOCK_MODE_FLAG_SILENT))
        {
            printf("%s:%zu:\n",file,line);
            printf(" Expected: %lu\n",expected);
            printf("      Got: %lu\n",v);
        }
        exit(-1);
    }
}
