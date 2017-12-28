#include "ioapic.h"
#include "kernel/console.h"
#include "acpi/tables.h"
#include "mm/mm.h"

using namespace kernel;
using kernel::console::printf;

static kernel::klist<kernel::ioapic> ioapics;

kernel::ioapic*
kernel::find_ioapic_for_acpi_interrupt(uint32_t acpi_interrupt)
{
    for (auto& ioa : klist_elems(ioapics,link))
    {
        if (ioa.acpi_interrupt_base <= acpi_interrupt &&
            acpi_interrupt < ioa.acpi_interrupt_base + ioa.interrupt_count)
        {
            return &ioa;
        }
    }
    return NULL;
}

kernel::ioapic::ioapic(uint8_t apic_id, uint64_t apic_addr,
    uint32_t acpi_interrupt_base):
        regs((ioapic_registers*)iomap(apic_addr)),
        apic_addr(apic_addr),
        interrupt_count(((read_reg(1) >> 16) & 0xFF) + 1),
        acpi_interrupt_base(acpi_interrupt_base),
        apic_id(apic_id)
{
    printf("IOAPIC_ID %u IOAPIC_ADDR 0x%08lX INT_RANGE [%u,%u)\n",
           apic_id,apic_addr,acpi_interrupt_base,
           acpi_interrupt_base+interrupt_count);

    // Mask all IOAPIC interrupts.
    for (size_t i=0; i<interrupt_count; ++i)
    {
        uint8_t index = 0x10 + i*2;
        uint32_t v    = read_reg(index);
        write_reg(v | 0x00010000,index);
    }
}

uint32_t
kernel::ioapic::read_reg(uint8_t index)
{
    regs->index = index;
    return regs->data;
}

void
kernel::ioapic::write_reg(uint32_t value, uint8_t index)
{
    regs->index = index;
    regs->data  = value;
}

void
kernel::init_ioapics()
{
    const sdt_header* h = find_acpi_table(MADT_SIG);
    kassert(h != NULL);

    const madt_table* madt = (const madt_table*)h;
    const madt_record* r   = madt->records;
    const madt_record* end = (const madt_record*)((uintptr_t)h + h->length);
    while (r < end)
    {
        if (r->type == MADT_TYPE_IOAPIC)
        {
            auto ioa = new ioapic(r->type1.io_apic_id,r->type1.io_apic_addr,
                                  r->type1.interrupt_base);
            ioapics.push_back(&ioa->link);
        }

        r = (const madt_record*)((uintptr_t)r + r->len);
    }
    kassert(r == end);
}
