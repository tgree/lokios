#include "../kassert.h"
#include <tmock/tmock.h>

void
kernel::halt() noexcept
{
    tmock::abort("kernel::halt() invoked");
}

void
kernel::panic(const char* s) noexcept
{
    tmock::abort(s);
}
