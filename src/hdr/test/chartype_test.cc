#include "../chartype.h"
#include <ctype.h>
#include <tmock/tmock.h>

class tmock_test
{
    TMOCK_TEST(test_chartype)
    {
        for (int i=0; i<256; ++i)
        {
            tmock::assert_equiv((bool)kisalnum(i), (bool)isalnum(i));
            tmock::assert_equiv((bool)kisalpha(i), (bool)isalpha(i));
            tmock::assert_equiv((bool)kiscntrl(i), (bool)iscntrl(i));
            tmock::assert_equiv((bool)kisdigit(i), (bool)isdigit(i));
            tmock::assert_equiv((bool)kisgraph(i), (bool)isgraph(i));
            tmock::assert_equiv((bool)kislower(i), (bool)islower(i));
            tmock::assert_equiv((bool)kisprint(i), (bool)isprint(i));
            tmock::assert_equiv((bool)kispunct(i), (bool)ispunct(i));
            tmock::assert_equiv((bool)kisspace(i), (bool)isspace(i));
            tmock::assert_equiv((bool)kisupper(i), (bool)isupper(i));
            tmock::assert_equiv((bool)kisxdigit(i),(bool)isxdigit(i));
            tmock::assert_equiv((bool)kisascii(i), (bool)isascii(i));
            tmock::assert_equiv((bool)kisblank(i), (bool)isblank(i));
        }
    }
};

TMOCK_MAIN();
