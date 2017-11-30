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
#define TCI_FLAG_SHOULD_FAIL        (1<<1)
#define TCI_FLAG_DID_FAIL           (1<<2)
#define TCI_RESULT_FLAGS (TCI_FLAG_FAILURE_EXPECTED | TCI_FLAG_SHOULD_FAIL)
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

#define TMOCK_MODE_FLAG_SILENT  (1<<0)  // Don't print expected vs. actual
        extern uint64_t mode_flags;
    }
}

#define _TMOCK_TEST(fn,flags) \
    static void fn();                                       \
    static tmock::internal::test_case_info                  \
        fn ## _test_case_info = {NULL,fn,#fn,flags};        \
    static tmock::internal::test_case_registrar             \
        fn ## _test_case_registrar(&fn ## _test_case_info); \
    static void fn()

// Define tests.  There is actually a matrix of test results and the defines
// below help you set up the expected behaviour of your test case.  You may
// have any of the following cases:
//
//  1. A test case that we expect will pass and SHOULD pass.  (Testing normal
//     functionality).
//  2. A test case that we expect will fail and SHOULD fail.  (Testing the
//     assert/abort paths actually assert/abort).
//  3. A test case that we expect will pass but SHOULDN'T pass.  (The code
//     under test is missing some sort of precondition check that should
//     trigger an abort or some other failure).
//  4. A test case that we expect will fail but SHOULDN'T fail.  (The code
//     under test is missing functionality and just aborts/asserts if you try
//     to exercise that functionality).
//
// We differentiate between what we expect the test to do and what the code
// SHOULD do so that we can track missing/incomplete functionality as part of
// our unittest output.
#define PASS_EXPECTED_SHOULD_PASS  0
#define FAIL_EXPECTED_SHOULD_FAIL \
    (TCI_FLAG_FAILURE_EXPECTED | TCI_FLAG_SHOULD_FAIL)
#define PASS_EXPECTED_SHOULD_FAIL TCI_FLAG_SHOULD_FAIL
#define FAIL_EXPECTED_SHOULD_PASS TCI_FLAG_FAILURE_EXPECTED

#define TMOCK_TEST(fn) \
    _TMOCK_TEST(fn,PASS_EXPECTED_SHOULD_PASS)
#define TMOCK_TEST_EXPECT_FAILURE(fn) \
    _TMOCK_TEST(fn,FAIL_EXPECTED_SHOULD_FAIL)
#define TMOCK_TEST_EXPECT_PASS_SHOULD_FAIL(fn) \
    _TMOCK_TEST(fn,PASS_EXPECTED_SHOULD_FAIL)
#define TMOCK_TEST_EXPECT_FAILURE_SHOULD_PASS(fn) \
    _TMOCK_TEST(fn,FAIL_EXPECTED_SHOULD_PASS)

#define TMOCK_MAIN() \
    int \
    main(int argc, const char* argv[]) \
    { \
        return tmock::run_tests(argc,argv); \
    }

#endif /* __TMOCK_H */
