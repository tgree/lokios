#include "../string_stream.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void
panic(const char* s) noexcept
{
    printf("Unexpected panic: %s\n",s);
    exit(-2);
}

static void
assert_string_equal(const char* s, const char* e)
{
    if (strcmp(s,e))
    {
        printf("Expected: '%s'\n",e);
        printf("     Got: '%s'\n",s);
        exit(-1);
    }
}

int
main(int argc, const char* argv[])
{
    fixed_string_stream<10> fs;
    fs.printf("Hello, world!");
    assert_string_equal(fs,"Hello, wo");
    printf("%s passed\n",argv[0]);
}
