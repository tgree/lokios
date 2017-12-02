#ifndef __KERNEL_E820_H
#define __KERNEL_E820_H

#include "kassert.h"
#include <stddef.h>

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
#define E820_TYPE_RAM_MASK              (1 << E820_TYPE_RAM)
#define E820_TYPE_RESERVED              (1 << E820_TYPE_RESERVED)
#define E820_TYPE_ACPI_RECLAIMABLE_MASK (1 << E820_TYPE_RECLAIMABLE)
#define E820_TYPE_ACPI_NVS_MASK         (1 << E820_TYPE_ACPI_NVS)
#define E820_TYPE_BAD_RAM_MASK          (1 << E820_TYPE_BAD_RAM)
#define E820_TYPE_ALL_MASK              0x0000003E

    struct e820_entry
    {
        uint64_t    base;
        uint64_t    len;
        e820_type   type;
        uint32_t    rsrv;
    };
    KASSERT(sizeof(e820_entry) == 24);

    struct e820_map
    {
        uint16_t    nentries;
        e820_entry  entries[];
    } __attribute__((packed));

    template<typename C>
    void get_e820_map(const e820_map* m, C& c,
                      uint64_t mask = E820_TYPE_ALL_MASK)
    {
        const e820_entry* e = m->entries;
        for (size_t i=0; i<m->nentries; ++i, ++e)
        {
            if (mask & (1 << e->type))
                c.push_back(*e);
        }
    }

    void
    parse_e820_map(const e820_map* m);
}

inline bool operator<(const kernel::e820_entry& l,
                      const kernel::e820_entry& r)
{
    if (l.base < r.base)
        return true;
    if (l.base > r.base)
        return false;

    if (l.len < r.len)
        return true;
    if (l.len > r.len)
        return false;

    return l.type < r.type;
}

#endif /* __KERNEL_E820_H */
