#include <stdint.h>
#include "e820.h"

struct kernel_args
{
    uint64_t    e820_base;
    uint64_t    vga_base;
};

int
main(const kernel_args* args)
{
    // The first 2M of RAM is now identity-mapped.  We are running in 64-bit
    // mode.  This is the point of no return; we can't go back to BIOS and we
    // can no longer make use of INT calls.  Demonstrate the standard success
    // code of turning the screen blue.
    uint64_t*   addr = (uint64_t*)args->vga_base;
    for (unsigned int i=0; i<500; ++i)
        addr[i] = 0x1F201F201F201F20;

    uint8_t*    chars = (uint8_t*)args->vga_base;
    chars[0] = 'L';
    chars[2] = 'o';
    chars[4] = 'k';
    chars[6] = 'i';
}
