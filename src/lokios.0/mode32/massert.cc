#include "massert.h"
#include "console.h"
#include "hdr/x86.h"
#include "kernel/kassert.h"

extern "C"
{
    void __NORETURN__ abort()
    {
        cpu_halt();
        for (;;)
            ;
    }
}

void
_abort(const char* f, int l)
{
    console::printf("ASSERTION FAILED: %s:%u\n",f,l);
    abort();
}

void
kernel::panic(const char* s, const char* f, unsigned int l) noexcept
{
    abort();
}
