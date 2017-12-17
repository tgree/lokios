#include "cxx_exception.h"

struct test_exception : public kernel::exception
{
    virtual const char* c_str() const
    {
        return "Test Exception";
    }
};

void
kernel::throw_test_exception()
{
    throw test_exception();
}
