#ifndef __KERNEL_IOAPIC_H
#define __KERNEL_IOAPIC_H

#include "kernel/kassert.h"
#include "kernel/types.h"
#include "k++/klist.h"
#include <stdint.h>

namespace kernel
{
    typedef fat_register<16,uint32_t> ioapic_register;

    struct ioapic_registers
    {
        ioapic_register index;      // 0xFEC00000
        ioapic_register data;       // 0xFEC00010
        uint8_t         rsrv[32];   // 0xFEC00020
        ioapic_register eoi;        // 0xFEC00040
    };
    KASSERT(sizeof(ioapic_registers) == 0x50);

    struct ioapic
    {
        klink               link;
        ioapic_registers*   regs;
        const uint64_t      apic_addr;
        const uint8_t       interrupt_count;
        const uint32_t      acpi_interrupt_base;
        const uint8_t       apic_id;

        uint32_t    read_reg(uint8_t index);
        void        write_reg(uint32_t value, uint8_t index);

        ioapic(uint8_t apic_id, uint64_t apic_addr,
               uint32_t acpi_interrupt_base);
    };

    void init_ioapics();
}

#endif /* __KERNEL_IOAPIC_H */
