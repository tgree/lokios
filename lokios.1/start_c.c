#include <stdint.h>

static void
_hlt()
{
    asm ("hlt;");
}

void
_entry_64(void)
{
    // The first 2M of RAM is now identity-mapped.  We are running in 64-bit
    // mode.  This is the point of no return; we can't go back to BIOS and we
    // can no longer make use of INT calls.  Demonstrate the standard success
    // code of turning the screen blue.
    uint64_t*   addr = (uint64_t*)0x000B8000;
    for (unsigned int i=0; i<500; ++i)
        addr[i] = 0x1F201F201F201F20;
    _hlt();
}
