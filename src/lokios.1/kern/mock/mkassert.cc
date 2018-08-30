#include "../kassert.h"
#include <tmock/tmock.h>

void
kernel::halt() noexcept
{
    mock("kernel::halt");
    for (;;)
        ;
}

extern "C" void
abort()
{
    mock("abort");
    for (;;)
        ;
}

void
kernel::panic(const char* s) noexcept
{
    mock("kernel::panic",s);
    for (;;)
        ;
}
