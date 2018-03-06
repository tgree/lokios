#ifndef __KERNEL_INTERRUPTS_MP_TABLES_H
#define __KERNEL_INTERRUPTS_MP_TABLES_H

#include "../kassert.h"
#include <stdint.h>

namespace kernel
{
#define MPFP_SIG 0x5F504D5F
    struct mpfp_struct
    {
        uint32_t    signature;
        uint32_t    physical_address_pointer;
        uint8_t     length_div_16;
        uint8_t     spec_rev;
        uint8_t     checksum;
        uint8_t     features[5];
    };
    KASSERT(sizeof(mpfp_struct) == 16);

    void init_mp_tables();
}

#endif /* __KERNEL_INTERRUPTS_MP_TABLES_H */
