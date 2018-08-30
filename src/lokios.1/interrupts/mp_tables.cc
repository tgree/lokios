#include "mp_tables.h"
#include "kern/console.h"
#include "mm/mm.h"
#include "k++/kmath.h"
#include "k++/checksum.h"
    
using kernel::console::printf;

static const kernel::mpfp_struct*
mpfp_search(dma_addr64 _base, size_t len)
{
    dma_addr64 end = _base + len;
    end = MIN(end,(dma_addr64)0x00100000);
    dma_addr64 start = kernel::round_up_pow2(_base,16);
    while (start < end)
    {
        uint32_t* v = (uint32_t*)kernel::phys_to_virt_maybe_0(start);
        if (*v == MPFP_SIG)
            return (const kernel::mpfp_struct*)v;
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
        mpfp = mpfp_search(phys_read<uint16_t>(0x40E),1024);

    // Try the last 1KB of system base memory.
    if (mpfp == NULL)
    {
        uint16_t base_mem_size_kb_minus_1 = phys_read<uint16_t>(0x413);
        uintptr_t base_mem_size_minus_1024 = base_mem_size_kb_minus_1*1024;
        mpfp = mpfp_search(base_mem_size_minus_1024,1024);
    }

    // Try the BIOS ROM area.
    if (mpfp == NULL)
        mpfp = mpfp_search(0xF0000,0x10000);

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
