#include "../sbrk.h"
#include <tmock/tmock.h>

dma_addr64
kernel::sbrk(size_t n)
{
    return (dma_addr64)mock("kernel::sbrk",n);
}

dma_addr64
kernel::get_sbrk()
{
    return (dma_addr64)mock("kernel::get_sbrk");
}
