#include "ioapic.h"
#include "console.h"
#include "acpi/tables.h"
#include "mm/mm.h"

using namespace kernel;

kernel::ioapic::ioapic(uint8_t apic_id, uint64_t apic_addr,
    uint32_t interrupt_base):
        regs((ioapic_registers*)iomap(apic_addr)),
        apic_addr(apic_addr),
        interrupt_base(interrupt_base),
        apic_id(apic_id)
{
    vga->printf("IOAPIC_ID %u IOAPIC_ADDR 0x%08lX INT_BASE %u\n",
                apic_id,apic_addr,interrupt_base);

    // Mask all IOAPIC interrupts.
    uint32_t ver = read_reg(1);
    uint32_t nints = ((ver >> 16) & 0xFF) + 1;
    for (size_t i=0; i<nints; ++i)
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
        if (r->type == 1)
        {
            new ioapic(r->type1.io_apic_id,r->type1.io_apic_addr,
                       r->type1.interrupt_base);
        }

        r = (const madt_record*)((uintptr_t)r + r->len);
    }
    kassert(r == end);
}
