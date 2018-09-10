#ifndef __KERNEL_VMM_DEVICE_H
#define __KERNEL_VMM_DEVICE_H

#include "k++/klist.h"

namespace vmm
{
    struct device
    {
        kernel::kdlink  link;
        const uint64_t  vm_addr;
        const size_t    len;

        virtual uint8_t read8(uint64_t vaddr) = 0;
        virtual void    write8(uint8_t val, uint64_t vaddr) = 0;

        constexpr device(uint64_t vm_addr, size_t len):
            vm_addr(vm_addr),
            len(len)
        {
        }
        virtual ~device() {}
    };
}

#endif /* __KERNEL_VMM_DEVICE_H */
