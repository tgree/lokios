#ifndef __KERNEL_VMM_VM_H
#define __KERNEL_VMM_VM_H

#include "ram_device.h"
#include "k++/klist.h"
#include "mm/slab.h"

namespace vmm
{
    struct exception {};
    struct device_conflict_error : public exception {};
    struct bus_error : public exception
    {
        uint64_t vm_addr;
        constexpr bus_error(uint64_t vm_addr):vm_addr(vm_addr) {}
    };

    struct machine
    {
        kernel::slab            devices_slab;
        kernel::kdlist<device>  devices;
        device*                 last_used_device;

        // Insert/remove devices.
        void insert_device(device* d);
        void remove_device(device* d);
        device* find_device_no_throw(uint64_t vm_addr);
        device* find_device(uint64_t vm_addr);

        // VM factory methods.
        ram_device* make_ram(uint64_t vm_addr, size_t len)
        {
            return devices_slab.alloc<ram_device>(vm_addr,len);
        }
        void delete_device(device* d)
        {
            devices_slab.free(d);
        }

        uint8_t     read8(uint64_t vaddr);
        uint16_t    read16(uint64_t vaddr);
        uint32_t    read32(uint64_t vaddr);
        uint64_t    read64(uint64_t vaddr);
        void        write8(uint8_t val, uint64_t vaddr);
        void        write16(uint16_t val, uint64_t vaddr);
        void        write32(uint32_t val, uint64_t vaddr);
        void        write64(uint64_t val, uint64_t vaddr);

        machine();
        ~machine();
    };
}

#endif /* __KERNEL_VMM_VM_H */
