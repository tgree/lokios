#include "rom_device.h"
#include "vm.h"

vmm::rom_device::rom_device(const void* image, uint64_t vm_addr, size_t len):
    device(vm_addr,len),
    image(image)
{
}

uint8_t
vmm::rom_device::read8(uint64_t vaddr)
{
    if (vaddr < vm_addr)
        throw bus_error{vaddr};
    if (vaddr >= vm_addr + len)
        throw bus_error{vaddr};

    return ((const uint8_t*)image)[vaddr - vm_addr];
}

void
vmm::rom_device::write8(uint8_t val, uint64_t vaddr)
{
    throw bus_error{vaddr};
}
