#include "../mm.h"
#include <tmock/tmock.h>

dma_addr64
kernel::virt_to_phys(const void* v)
{
    return (dma_addr64)mock("kernel::virt_to_phys",v);
}

void*
kernel::phys_to_virt_maybe_0(dma_addr64 p)
{
    return (void*)mock("kernel::phys_to_virt_maybe_0",p);
}

void*
kernel::phys_to_virt(dma_addr64 p)
{
    return (void*)mock("kernel::phys_to_virt",p);
}

dma_addr64
kernel::xlate(const void* v)
{
    return (dma_addr64)mock("kernel::xlate",v);
}
