/*
 * According to the ICH10 doc the PIC has the following ISA IRQs:
 *
 *  IRQ#    Internal device
 *  ----    ---------------
 *  0       Internal Timer/ Counter 0 output / HPET #0
 *  1       Keyboard
 *  2       Slave controller INTR output
 *  3       Serial Port A
 *  4       Serial Port B
 *  5       Parallel Port / Generic
 *  6       Floppy Disk
 *  7       Parallel Port / Generic
 *  8       Internal RTC / HPET #1
 *  9       Generic
 *  10      Generic
 *  11      Generic / HPET #2
 *  12      PS/2 Mouse / HPET #3
 *  13      Internal
 *  14      SATA Primary (legacy mode)
 *  15      SATA Secondary (legacy mode)
 *
 * When we are using IOAPICs, we need to identity-map the first 16 IOAPIC
 * interrupts to these legacy IRQs since they map to the same devices.
 * However, we also have to consult the ACPI tables which may have some
 * overrides that change the identity-map around.
 *
 * For ISA interrupts: Assume edge trigger, active high unless there is an
 * override.
 */
#ifndef __KERNEL_INTERRUPTS_ROUTING_H
#define __KERNEL_INTERRUPTS_ROUTING_H

#include <stdint.h>

namespace kernel
{
    struct ioapic;

    // Values for acpi_flags field.
#define ACPI_FLAG_POLARITY_MASK     0x03
#define ACPI_FLAG_POLARITY_DEFAULT  0x00
#define ACPI_FLAG_POLARITY_HIGH     0x01
#define ACPI_FLAG_POLARITY_LOW      0x03
#define ACPI_FLAG_TRIGGER_MASK      0x0C
#define ACPI_FLAG_TRIGGER_DEFAULT   0x00
#define ACPI_FLAG_TRIGGER_EDGE      0x04
#define ACPI_FLAG_TRIGGER_LEVEL     0x0C

    struct irq_route
    {
        const uint8_t   irq;
        const char*     name;
        uint32_t        acpi_interrupt_num;

        kernel::ioapic* ioapic;
        uint32_t        apic_index;
        uint16_t        acpi_flags;
    };

    extern irq_route irq_routes[16];

    void init_irq_routing();
}

#endif /* __KERNEL_INTERRUPTS_ROUTING_H */
