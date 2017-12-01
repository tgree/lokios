#ifndef __KERNEL_E820_H
#define __KERNEL_E820_H

#include <stdint.h>

namespace kernel
{
    enum e820_type : uint32_t
    {
        E820_TYPE_RAM              = 1,
        E820_TYPE_RESERVED         = 2,
        E820_TYPE_ACPI_RECLAIMABLE = 3,
        E820_TYPE_ACPI_NVS         = 4,
        E820_TYPE_BAD_RAM          = 5,
    };

    struct e820_entry
    {
        uint64_t    base;
        uint64_t    len;
        e820_type   type;
        uint32_t    rsrv;
    };

    struct e820_map
    {
        uint16_t    nentries;
        e820_entry  entries[];
    } __attribute__((packed));
}

#endif /* __KERNEL_E820_H */
