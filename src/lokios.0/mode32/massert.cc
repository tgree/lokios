#include "massert.h"
#include "console.h"
#include "hdr/x86.h"

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
m32_abort(const char* f, int l)
{
    console::printf("ASSERTION FAILED: %s:%u\n",f,l);
    abort();
}
