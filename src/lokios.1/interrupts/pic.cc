#include "pic.h"
#include "acpi/tables.h"
#include "kernel/console.h"
#include "kernel/x86.h"

using kernel::console::printf;

void
kernel::init_pic()
{
    // Check the MADT table to see if we have a PIC.
    const sdt_header* h = find_acpi_table(MADT_SIG);
    kassert(h != NULL);
    const madt_table* madt = (const madt_table*)h;
    if (!(madt->flags & (1 << 0)))
    {
        printf("MADT indicates no PIC present.\n");
        return;
    }
    printf("MADT indicates PIC present, masking interrupts.\n");

    // Mask all interrupts in the master PIC.
    outb(0xFF,0x21);

    // Mask all interrupts in the slave PIC.
    outb(0xFF,0xA1);
}
