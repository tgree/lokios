#include "../string_stream.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BROKEN_TEST 0

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

static void
test_fixed_string_stream()
{
    fixed_string_stream<10> fs;
    fs.printf("Hello, world!");
    assert_string_equal(fs,"Hello, wo");
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
        assert_string_equal(fs,expected);
        assert_string_equal(expected,tc.expected);
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
#if BROKEN_TEST
    {0x2468ABCD,"%08X","2468ABCD"},
    {0x12EF,    "%X",  "12EF"},
    {0x789AB,   "%08X","000789AB"},
#endif
};
static fmt_test_case<int> int_tests[] = {
    {12345,  "%d", "12345"},
#if BROKEN_TEST
    {-12345, "%d", "-12345"},
#endif
};

static void
test_fixed_hex()
{
    run_fmt_tests(uint_tests);
    run_fmt_tests(int_tests);
}

#define TEST(f) {f,#f}
struct test_case
{
    void (*func)();
    const char* name;
};
static test_case tests[] =
{
    TEST(test_fixed_string_stream),
    TEST(test_fixed_hex),
};

int
main(int argc, const char* argv[])
{
    for (test_case& tc : tests)
    {
        tc.func();
        printf("%s:%s passed\n",argv[0],tc.name);
    }
}
