#include "vm.h"

using kernel::_kassert;

vmm::machine::machine():
    devices_slab(MAX(sizeof(ram_device))),
    last_used_device(NULL)
{
}

vmm::machine::~machine()
{
    while (!devices.empty())
    {
        auto* d = klist_front(devices,link);
        devices.pop_front();
        devices_slab.free(d);
    }
}

void
vmm::machine::insert_device(vmm::device* d)
{
    uint64_t prev_end = 0;
    auto iter         = klist_begin(devices,link);
    while (iter != klist_end(devices,link))
    {
        if (d->vm_addr <= (*iter).vm_addr)
        {
            if ((*iter).vm_addr < d->vm_addr + d->len)
                throw device_conflict_error();
            break;
        }
        prev_end = (*iter).vm_addr + (*iter).len;
        ++iter;
    }
    if (prev_end > d->vm_addr)
        throw device_conflict_error();
    d->link.insert_before(const_cast<kernel::kdlink_leaks*>(iter.pos));
}

void
vmm::machine::remove_device(vmm::device* d)
{
    d->link.unlink();
    if (last_used_device == d)
        last_used_device = NULL;
}

vmm::device*
vmm::machine::find_device_no_throw(uint64_t vm_addr)
{
    // Principle of locality first.
    if (last_used_device)
    {
        if (last_used_device->vm_addr <= vm_addr &&
            vm_addr < last_used_device->vm_addr + last_used_device->len)
        {
            return last_used_device;
        }
    }

    // Search.
    for (auto& d : klist_elems(devices,link))
    {
        if (vm_addr < d.vm_addr + d.len)
        {
            last_used_device = &d;
            return &d;
        }
    }

    return NULL;
}

vmm::device*
vmm::machine::find_device(uint64_t vm_addr)
{
    return find_device_no_throw(vm_addr) ?: throw bus_error(vm_addr);
}

uint8_t
vmm::machine::read8(uint64_t vaddr)
{
    return find_device(vaddr)->read8(vaddr);
}

uint16_t
vmm::machine::read16(uint64_t vaddr)
{
    return ((uint16_t)read8(vaddr + 0) << 0) | 
           ((uint16_t)read8(vaddr + 1) << 8);
}

uint32_t
vmm::machine::read32(uint64_t vaddr)
{
    return ((uint32_t)read16(vaddr + 0) << 0) | 
           ((uint32_t)read16(vaddr + 2) << 16);
}

uint64_t
vmm::machine::read64(uint64_t vaddr)
{
    return ((uint64_t)read32(vaddr + 0) << 0) | 
           ((uint64_t)read32(vaddr + 4) << 32);
}

void
vmm::machine::write8(uint8_t val, uint64_t vaddr)
{
    return find_device(vaddr)->write8(val,vaddr);
}

void
vmm::machine::write16(uint16_t val, uint64_t vaddr)
{
    write8((val >> 0),vaddr + 0);
    write8((val >> 8),vaddr + 1);
}

void
vmm::machine::write32(uint32_t val, uint64_t vaddr)
{
    write16((val >>  0),vaddr + 0);
    write16((val >> 16),vaddr + 2);
}

void
vmm::machine::write64(uint64_t val, uint64_t vaddr)
{
    write32((val >>  0),vaddr + 0);
    write32((val >> 32),vaddr + 4);
}
