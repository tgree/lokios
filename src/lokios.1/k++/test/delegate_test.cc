#include "../delegate.h"
#include "tmock/tmock.h"

using namespace kernel;

struct base
{
    delegate<int(char,long)> mc;

    int foo(char a, long b)
    {
        return a*b;
    }

    base():mc(method_delegate(foo)) {}
};

#if 0
// This doesn't compile for some reason that I have yet to figure out.
struct derived_using_base : public base
{
    delegate<int(char,long)> mc2;

    derived_using_base():mc2(method_delegate(foo)) {}
};
#endif

struct derived_using_derived : public base
{
    delegate<int(long,char)> mc2;

    int bar(long a, char b)
    {
        return a + b;
    }

    derived_using_derived():mc2(method_delegate(bar)) {}
};

static int baz(int a, long b, char c)
{
    return a+b+c;
}

class tmock_test
{
    TMOCK_TEST(test_call_base)
    {
        base b;
        TASSERT(b.mc(3,4) == 12);
    }

#if 0
    TMOCK_TEST(test_call_base_from_derived)
    {
        derived_using_base db;
        TASSERT(db.mc2(3,4) == 12);
    }
#endif

    TMOCK_TEST(test_call_derived_from_derived)
    {
        derived_using_derived dd;
        TASSERT(dd.mc2(3,4) == 7);
    }

    TMOCK_TEST(test_call_func)
    {
        delegate<int(int,long,char)> d(func_delegate(baz));
    }
};

TMOCK_MAIN();
