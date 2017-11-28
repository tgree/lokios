#include "tmock.h"
#include <string.h>
#include <stdio.h>

void
tmock::assert_equiv(const char* s, const char* expected)
{
    if (strcmp(s,expected))
    {
        printf("Expected: '%s'\n",expected);
        printf("     Got: '%s'\n",s);
        exit(-1);
    }
}

void
tmock::assert_equiv(uint16_t v, uint16_t expected)
{
    if (v != expected)
    {
        printf("Expected: 0x%04X\n",expected);
        printf("     Got: 0x%04X\n",v);
        exit(-1);
    }
}
