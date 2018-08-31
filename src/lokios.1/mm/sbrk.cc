#include "sbrk.h"
#include "kern/spinlock.h"

static kernel::spinlock sbrklock;
static dma_addr64 _brk = KERNEL_SBRK_END;
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
kernel::freeze_sbrk()
{
    sbrk_frozen = true;
}

dma_addr64
kernel::get_sbrk_limit()
{
    return _brklim;
}

void
kernel::init_sbrk(dma_addr64 pos)
{
    kassert(pos <= _brklim);
    _brk = pos;
}
