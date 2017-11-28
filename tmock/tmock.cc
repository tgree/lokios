#include "tmock.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct test_case
{
    void        (*fn)();
    const char* name;
};

static tmock::internal::test_case_info* test_cases;

tmock::internal::test_case_registrar::test_case_registrar(
    tmock::internal::test_case_info* tci)
{
    tci->next = NULL;

    if (!test_cases)
    {
        test_cases = tci;
        return;
    }

    tmock::internal::test_case_info* pos = test_cases;
    while (pos->next)
        pos = pos->next;

    pos->next = tci;
}

int
tmock::run_tests(int argc, const char* argv[])
{
    for (tmock::internal::test_case_info* tci = test_cases;
         tci;
         tci = tci->next)
    {
        tci->fn();
        printf("%s:%s passed\n",argv[0],tci->name);
    }
    return 0;
}

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
