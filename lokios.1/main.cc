#include <stdint.h>

int
main(int argc, const char* argv[])
{
    // The first 2M of RAM is now identity-mapped.  We are running in 64-bit
    // mode.  This is the point of no return; we can't go back to BIOS and we
    // can no longer make use of INT calls.  Demonstrate the standard success
    // code of turning the screen blue.
    uint64_t*   addr = (uint64_t*)0x000B8000;
    for (unsigned int i=0; i<500; ++i)
        addr[i] = 0x1F201F201F201F20;

    uint8_t*    chars = (uint8_t*)0x000B8000;
    chars[0] = 'L';
    chars[2] = 'o';
    chars[4] = 'k';
    chars[6] = 'i';
}
