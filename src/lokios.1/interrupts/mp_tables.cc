#include "mp_tables.h"
#include "../console.h"
#include "k++/checksum.h"
    
using kernel::console::printf;

static const kernel::mpfp_struct*
mpfp_search(const void* _base, size_t len)
{
    const uint32_t* end = (const uint32_t*)((uintptr_t)_base + len);
    end = kernel::min(end,(const uint32_t*)0x00100000);
    const uint32_t* start = kernel::round_up_pow2((const uint32_t*)_base,16);
    while (start < end)
    {
        if (*start == MPFP_SIG)
            return (const kernel::mpfp_struct*)start;
        start += 4;
    }
    return NULL;
}
    
void
kernel::init_mp_tables()
{
    const mpfp_struct* mpfp = NULL;

    // Start with the BIOS EBDA area.
    if (mpfp == NULL)
    {
        const void* ebdap = (const void*)(uintptr_t)*(uint16_t*)0x40E;
        mpfp = mpfp_search(ebdap,1024);
    }

    // Try the last 1KB of system base memory.
    if (mpfp == NULL)
    {
        uint16_t base_mem_size_kb_minus_1 = *(uint16_t*)0x413;
        uintptr_t base_mem_size_minus_1024 = base_mem_size_kb_minus_1*1024;
        mpfp = mpfp_search((const void*)base_mem_size_minus_1024,1024);
    }

    // Try the BIOS ROM area.
    if (mpfp == NULL)
        mpfp = mpfp_search((const void*)0xF0000,0x10000);

    if (mpfp)
    {
        if (checksum<uint8_t>(mpfp,mpfp->length_div_16*16) != 0)
            printf("MPFP found at %p but checksum is invalid.\n",mpfp);
        else
        {
            printf("%p: MPFP rev 0x%02X "
                   "features 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
                   mpfp,mpfp->spec_rev,mpfp->features[0],mpfp->features[1],
                   mpfp->features[2],mpfp->features[3],mpfp->features[4]);
        }
    }
    else
        printf("MPFP not found.\n");
}
