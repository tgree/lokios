#include "../kassert.h"
#include <tmock/tmock.h>

void
kernel::halt() noexcept
{
    tmock::abort("kernel::halt() invoked");
}

void
kernel::panic(const char* s, const char* f, unsigned int l) noexcept
{
    tmock::abort(s,f,l);
}
