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

