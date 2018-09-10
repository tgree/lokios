#include "ram_device.h"
#include "vm.h"

vmm::ram_device::ram_device(uint64_t vm_addr, size_t len):
    device(vm_addr,len),
    bb(len)
{
    memset(bb.addr,0,bb.len);
}

uint8_t
vmm::ram_device::read8(uint64_t vaddr)
{
    if (vaddr < vm_addr)
        throw bus_error{vaddr};
    if (vaddr >= vm_addr + len)
        throw bus_error{vaddr};

    return ((uint8_t*)bb.addr)[vaddr - vm_addr];
}

void
vmm::ram_device::write8(uint8_t val, uint64_t vaddr)
{
    if (vaddr < vm_addr)
        throw bus_error{vaddr};
    if (vaddr >= vm_addr + len)
        throw bus_error{vaddr};

    ((uint8_t*)bb.addr)[vaddr - vm_addr] = val;
}
