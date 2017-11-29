#ifndef __KERNEL_E820_H
#define __KERNEL_E820_H

#include <stdint.h>

struct e820_entry
{
    uint64_t    base;
    uint64_t    len;
    uint32_t    type;
    uint32_t    rsrv;
};

struct e820_map
{
    uint16_t    end;
    e820_entry  entries[];
} __attribute__((packed));

#endif /* __KERNEL_E820_H */
