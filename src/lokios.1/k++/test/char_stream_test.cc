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
    kernel::string_stream ss(buf2,n);
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

class tmock_test
{
    TMOCK_TEST(test_fixed_string_stream)
    {
        kernel::fixed_string_stream<10> fs;
        fs.printf("Hello, world!");
        tmock::assert_equiv(fs,"Hello, wo");
    }

    TMOCK_TEST(test_fmt_unsigned_int)
    {
        static fmt_test_case<unsigned int> uint_tests[] = {
            {12345,     "%u"  },
            {12345,     "%8u" },
            {12345,     "%08u"},
            {12345,     "%+u" },
            {12345,     "% u" },
            {12345,     "%08d"},
            {0x2468ABCD,"%08X"},
            {0x12EF,    "%X"  },
            {0x789AB,   "%08X"},
            {0x2468ABCD,"%08x"},
            {0x12EF,    "%x"  },
            {0x789AB,   "%08x"},
        };
        run_fmt_tests(uint_tests);
    }

    TMOCK_TEST(test_fmt_positive_int)
    {
        static fmt_test_case<int> int_tests[] = {
            {12345,"%d"},
            {2222, "%08d"},
            {4321, "%-08d"},
            {98765,"%+d"},
            {1357, "% d"},
            {1357, "%+ d"},
            {11111,"%+8d"},
            {2222, "% 8d"},
            {3333, "%+ 8d"},
            {44444,"%-+8d"},
            {5555, "%- 8d"},
            {6666, "%-+ 8d"},
            {7777, "%+ 1d"},
        };
        run_fmt_tests(int_tests);
    }

    TMOCK_TEST(test_fmt_negative_int)
    {
        static fmt_test_case<int> int_tests[] = {
            {-12345,"%d"},
            {-2222, "%08d"},
            {-4321, "%-08d"},
            {-4321, "%- 8d"},
            {-1,    "%1d"},
            {-100,  "%1d"},
            {-123,  "%+d"},
        };
        run_fmt_tests(int_tests);
    }

    TMOCK_TEST(test_fmt_positive_octal_int)
    {
        static fmt_test_case<int> int_tests[] = {
            {12345,"%o"},
            {2222, "%08o"},
            {4321, "%-08o"},
            {98765,"%+o"},
            {1357, "% o"},
            {1357, "%+ o"},
            {11111,"%+8o"},
            {2222, "% 8o"},
            {3333, "%+ 8o"},
            {44444,"%-+8o"},
            {5555, "%- 8o"},
            {6666, "%-+ 8o"},
            {7777, "%+ 1o"},
        };
        run_fmt_tests(int_tests);
    }

    TMOCK_TEST(test_fmt_string)
    {
        static fmt_test_case<const char*> string_tests[] = {
            {"hello", "%s"    },
            {"asdf",  "%8s"   },
            {"asdf",  "%-8s"  },
            {"ohayoo","%.3s"  },
            {"ohayoo","%.20s" },
            {"ohayoo","%8.3s" },
            {"ohayoo","%-8.3s"},
            {"hello", "%8.7s" },
            {"hello", "-%8.7s"},
        };
        run_fmt_tests(string_tests);
    }

    TMOCK_TEST(test_random_hex_strings)
    {
        test_random_string("Hello 0x%08X %p asdf\n",0xa1b2c3d4,(void*)12345);
        test_random_string("%.0d",0);
        test_random_string("%.0d",12345);
        test_random_string("%016lX",(uint64_t)0);
        test_random_string("%02X",(char)0xE0);
    }

    TMOCK_TEST(test_random_chars)
    {
        test_random_string("Hello %c 1234",'T');
    }

    TMOCK_TEST(test_ksprintf)
    {
        char c[15];
        kernel::ksprintf(c,"hello %s %s","number","one");
        tmock::assert_equiv(c,"hello number o");
    }
};

TMOCK_MAIN();
