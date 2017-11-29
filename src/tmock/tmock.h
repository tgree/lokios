#ifndef __TMOCK_H
#define __TMOCK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace tmock
{
    // Assert that two objects are equivalent.  For values it is just a simple
    // comparison with ==.  For things like const char* it will do a string
    // comparison.
    void assert_equiv(const char* s, const char* expected);
    void assert_equiv(uint16_t v, uint16_t expected);

    int run_tests(int argc, const char* argv[]);

    namespace internal
    {
#define TCI_FLAG_FAILURE_EXPECTED   (1<<0)
        struct test_case_info
        {
            test_case_info*     next;
            void                (*fn)();
            const char*         name;
            unsigned long       flags;
            pid_t               pid;
        };

        struct test_case_registrar
        {
            test_case_registrar(test_case_info* tci);
        };
    }
}

#define _TMOCK_TEST(fn,flags) \
    static void fn();                                       \
    static tmock::internal::test_case_info                  \
        fn ## _test_case_info = {NULL,fn,#fn,flags};        \
    static tmock::internal::test_case_registrar             \
        fn ## _test_case_registrar(&fn ## _test_case_info); \
    static void fn()

#define TMOCK_TEST(fn) _TMOCK_TEST(fn,0)
#define TMOCK_TEST_EXPECT_FAILURE(fn) _TMOCK_TEST(fn,TCI_FLAG_FAILURE_EXPECTED)

#define TMOCK_MAIN() \
    int \
    main(int argc, const char* argv[]) \
    { \
        return tmock::run_tests(argc,argv); \
    }

#endif /* __TMOCK_H */
