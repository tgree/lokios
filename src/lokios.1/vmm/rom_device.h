#ifndef __KERNEL_VMM_ROM_DEVICE_H
#define __KERNEL_VMM_ROM_DEVICE_H

#include "device.h"
#include "mm/buddy.h"

namespace vmm
{
    struct rom_device : public device
    {
        const void* image;

        virtual uint8_t read8(uint64_t vaddr) override;
        virtual void    write8(uint8_t val, uint64_t vaddr) override;

        rom_device(const void* image, uint64_t vm_addr, size_t len);
    };
}

#endif /* __KERNEL_VMM_ROM_DEVICE_H */
