#ifndef __KERNEL_VMM_RAM_DEVICE_H
#define __KERNEL_VMM_RAM_DEVICE_H

#include "device.h"
#include "mm/buddy.h"

namespace vmm
{
    struct ram_device : public device
    {
        kernel::buddy_block bb;

        virtual uint8_t read8(uint64_t vaddr) override;
        virtual void    write8(uint8_t val, uint64_t vaddr) override;

        ram_device(uint64_t vm_addr, size_t len);
    };
}

#endif /* __KERNEL_VMM_RAM_DEVICE_H */
