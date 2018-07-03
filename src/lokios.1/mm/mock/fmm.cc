#include "../mm.h"

dma_addr64
kernel::virt_to_phys(const void* v)
{
    return (dma_addr64)v;
}

void*
kernel::phys_to_virt_maybe_0(dma_addr64 p)
{
    return (void*)p;
}

void*
kernel::phys_to_virt(dma_addr64 p)
{
    kassert(p != 0);
    return phys_to_virt_maybe_0(p);
}

dma_addr64
kernel::xlate(const void* v)
{
    return (dma_addr64)v;
}
