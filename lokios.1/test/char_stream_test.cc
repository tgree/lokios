#include "../string_stream.h"
#include "tmock/tmock.h"

#include <stdio.h>
#include <stdlib.h>

#define BROKEN_TEST 0

void
panic(const char* s) noexcept
{
    printf("Unexpected panic: %s\n",s);
    exit(-2);
}

TMOCK_TEST(test_fixed_string_stream)
{
    fixed_string_stream<10> fs;
    fs.printf("Hello, world!");
    tmock::assert_equiv(fs,"Hello, wo");
}

template<typename T, size_t N>
static void run_fmt_tests(T (&tests)[N])
{
    for (T& tc : tests)
    {
        fixed_string_stream<20> fs;
        char expected[20];

        fs.printf(tc.fmt,tc.value);
        snprintf(expected,sizeof(expected),tc.fmt,tc.value);
        tmock::assert_equiv(fs,expected);
        tmock::assert_equiv(expected,tc.expected);
    }
}

template<typename T>
struct fmt_test_case
{
    T               value;
    const char*     fmt;
    const char*     expected;
};
static fmt_test_case<unsigned int> uint_tests[] = {
    {12345,     "%u",  "12345"},
    {12345,     "%8u", "   12345"},
    {12345,     "%08u","00012345"},
    {12345,     "%08d","00012345"},
    {0x2468ABCD,"%08X","2468ABCD"},
    {0x12EF,    "%X",  "12EF"},
    {0x789AB,   "%08X","000789AB"},
};
static fmt_test_case<int> int_tests[] = {
    {12345,  "%d", "12345"},
#if BROKEN_TEST
    {-12345, "%d", "-12345"},
#endif
};

TMOCK_TEST(test_fmt_integral_types)
{
    run_fmt_tests(uint_tests);
    run_fmt_tests(int_tests);
}

int
main(int argc, const char* argv[])
{
    return tmock::run_tests(argc,argv);
}
