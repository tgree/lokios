#include "../libc.h"
#include "tmock/tmock.h"
#include <stdio.h>
#include <string.h>

class tmock_test
{
    TMOCK_TEST(test_sprintf)
    {
        char buf[256];
        sprintf(buf,"hello %u %s \n",123,"world");
        TASSERT(!strcmp(buf,"hello 123 world \n"));
    }
};

TMOCK_MAIN();
