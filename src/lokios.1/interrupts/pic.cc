#include "pic.h"
#include "kernel/x86.h"

void
kernel::init_pic()
{
    // Mask all interrupts in the master PIC.
    outb(0xFF,0x21);

    // Mask all interrupts in the slave PIC.
    outb(0xFF,0xA1);
}
