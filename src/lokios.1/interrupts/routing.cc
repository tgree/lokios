#include "routing.h"
#include "ioapic.h"
#include "acpi/tables.h"
#include "kernel/console.h"
#include "k++/kmath.h"
#include <stddef.h>

using kernel::console::printf;

kernel::irq_route kernel::irq_routes[16] = {
    {0, "PIT"},
    {1, "Keyboard"},
    {2, "8259 Cascade"},
    {3, "Serial Port A"},
    {4, "Serial Port B"},
    {5, "Parallel Port"},
    {6, "Floppy Disk"},
    {7, "Parallel Port"},
    {8, "RTC"},
    {9, "Generic"},
    {10,"Generic"},
    {11,"Generic"},
    {12,"PS/2 Mouse"},
    {13,"Internal"},
    {14,"SATA Primary"},
    {15,"SATA Secondary"},
};

void
kernel::init_irq_routing()
{
    // Start with an identity-map.
    for (size_t i=0; i<nelems(irq_routes); ++i)
    {
        ioapic* ioa = find_ioapic_for_acpi_interrupt(i);
        if (!ioa)
            continue;

        irq_routes[i].acpi_interrupt_num = i;
        irq_routes[i].ioapic             = ioa;
        irq_routes[i].apic_index         = i - ioa->acpi_interrupt_base;
        irq_routes[i].acpi_flags         = ACPI_FLAG_POLARITY_HIGH |
                                           ACPI_FLAG_TRIGGER_EDGE;
    }

    // Walk the MADT table looking for interrupt overrides.
    const sdt_header* h = find_acpi_table(MADT_SIG);
    kassert(h != NULL);
    const madt_table* madt = (const madt_table*)h;
    for (auto& r : *madt)
    {
        if (r.type == MADT_TYPE_INTERRUPT_OVERRIDE && r.type2.bus_source == 0)
        {
            printf("BUS_SRC %u IRQ_SRC %u INT_NUM %u FL 0x%04X\n",
                   r.type2.bus_source,r.type2.irq_source,
                   r.type2.interrupt_num,r.type2.flags);

            // We are going to assume that interrupts don't map to the same
            // IOAPIC pin.  So, if there is an interrupt override then we
            // assume that whatever device was connected to that pin doesn't
            // get an interrupt anymore (and presumably there is another
            // override for it later if that device even exists).
            auto* ioa = find_ioapic_for_acpi_interrupt(r.type2.interrupt_num);
            kassert(ioa != NULL);
            auto* irqr    = &irq_routes[r.type2.irq_source];
            uint32_t irqn = r.type2.interrupt_num;
            for (auto& ir : irq_routes)
            {
                if (ir.ioapic == ioa && ir.acpi_interrupt_num == irqn)
                    ir.ioapic = NULL;
            }

            // Remap the interrupt.
            irqr->acpi_interrupt_num = irqn;
            irqr->ioapic             = ioa;
            irqr->apic_index         = irqn - ioa->acpi_interrupt_base;
            irqr->acpi_flags         = r.type2.flags;
            if (!(irqr->acpi_flags & ACPI_FLAG_POLARITY_MASK))
                irqr->acpi_flags |= ACPI_FLAG_POLARITY_HIGH;
            if (!(irqr->acpi_flags & ACPI_FLAG_TRIGGER_MASK))
                irqr->acpi_flags |= ACPI_FLAG_TRIGGER_EDGE;
        }
    }

    // Dump the final table.
    for (auto& ir : irq_routes)
    {
        if (!ir.ioapic)
        {
            printf("%2u: %14s ->\n",ir.irq,ir.name);
            continue;
        }
        printf("%2u: %14s -> IOAPIC 0x%016lX:%-2u 0x%04X\n",
               ir.irq,ir.name,ir.ioapic->apic_addr,ir.apic_index,ir.acpi_flags);
    }
}
