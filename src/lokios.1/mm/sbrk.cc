#include "sbrk.h"
#include "kernel/spinlock.h"
#include "kernel/kassert.h"
#include "kernel/image.h"

extern char _kernel_phys_base[];

static kernel::spinlock sbrklock;
static dma_addr64 _brk;
static dma_addr64 _brklim = KERNEL_SBRK_END;
static bool sbrk_frozen = false;

dma_addr64
kernel::sbrk(size_t n)
{
    dma_addr64 p;
    dma_addr64 new_brk;

    if (sbrk_frozen)
        return 0;

    n = round_up_pow2(n,16);
    with (sbrklock)
    {
        p    = _brk;
        _brk = new_brk = p + n;
    }
    kernel::kassert(new_brk <= _brklim);
    return p;
}

dma_addr64
kernel::get_sbrk()
{
    return _brk;
}

void
kernel::set_sbrk(dma_addr64 pos)
{
    _brk = pos;
}

void
kernel::freeze_sbrk()
{
    sbrk_frozen = true;
}

void
kernel::set_sbrk_limit(dma_addr64 new_lim)
{
    kernel::kassert(new_lim <= _brklim);
    _brklim = new_lim;
}

dma_addr64
kernel::get_sbrk_limit()
{
    return _brklim;
}

void
kernel::init_sbrk()
{
    _brk = (dma_addr64)_kernel_phys_base + image_header->num_sectors*512;
}
