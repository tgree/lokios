#ifndef __TMOCK_H
#define __TMOCK_H

#include <stdio.h>
#include <stdlib.h>

namespace tmock
{
    // Assert that two objects are equivalent.  For values it is just a simple
    // comparison with ==.  For things like const char* it will do a string
    // comparison.
    void assert_equiv(const char* s, const char* expected);

    int run_tests(int argc, const char* argv[]);

    namespace internal
    {
        struct test_case_info
        {
            test_case_info*     next;
            void                (*fn)();
            const char*         name;
        };

        struct test_case_registrar
        {
            test_case_registrar(test_case_info* tci);
        };
    }
}

#define TMOCK_TEST(fn) \
    static void fn();                                       \
    static tmock::internal::test_case_info                  \
        fn ## _test_case_info = {NULL,fn,#fn};              \
    static tmock::internal::test_case_registrar             \
        fn ## _test_case_registrar(&fn ## _test_case_info); \
    static void fn()

#endif /* __TMOCK_H */
