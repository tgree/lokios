#include "../string_stream.h"
#include "tmock/tmock.h"

#include <stdio.h>
#include <stdlib.h>

__attribute__((format(printf,1,2)))
static void
test_random_string(const char* fmt, ...)
{
    char* buf;
    va_list ap;

    va_start(ap,fmt);
    int n = vasprintf(&buf,fmt,ap) + 1;
    va_end(ap);

    char* buf2 = (char*)malloc(n);
    string_stream ss(buf2,n);
    va_start(ap,fmt);
    ss.vprintf(fmt,ap);
    va_end(ap);

    tmock::assert_equiv(buf2,buf);

    free(buf2);
    free(buf);
}

template<typename T>
struct fmt_test_case
{
    T               value;
    const char*     fmt;
};

template<typename T, size_t N>
static void run_fmt_tests(T (&tests)[N])
{
    for (T& tc : tests)
        test_random_string(tc.fmt,tc.value);
}

void
panic(const char* s) noexcept
{
    abort();
}

TMOCK_TEST(test_fixed_string_stream)
{
    fixed_string_stream<10> fs;
    fs.printf("Hello, world!");
    tmock::assert_equiv(fs,"Hello, wo");
}

TMOCK_TEST(test_fmt_unsigned_int)
{
    static fmt_test_case<unsigned int> uint_tests[] = {
        {12345,     "%u"  },
        {12345,     "%8u" },
        {12345,     "%08u"},
        {12345,     "%08d"},
        {0x2468ABCD,"%08X"},
        {0x12EF,    "%X"  },
        {0x789AB,   "%08X"},
    };
    run_fmt_tests(uint_tests);
}

TMOCK_TEST(test_fmt_positive_int)
{
    static fmt_test_case<int> int_tests[] = {
        {12345,"%d"},
    };
    run_fmt_tests(int_tests);
}

TMOCK_TEST_EXPECT_FAILURE(test_fmt_negative_int)
{
    static fmt_test_case<int> int_tests[] = {
        {-12345,"%d"},
    };
    run_fmt_tests(int_tests);
}

TMOCK_MAIN();