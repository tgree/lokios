#include "kassert.h"
#include "kernel_args.h"
#include "vga.h"
#include "serial.h"
#include <stdlib.h>

void
kernel::halt() noexcept
{
    vga_write(74,24,"Halted");
    serial_write("Halted\n");
    for (;;)
        asm ("hlt;");
}

extern "C" void
abort()
{
    // Hilite the entire screen red.
    kernel::vga_set_flags(0x4F00);

    // Let the serial console know.
    kernel::serial_write("ABORT\n");

    // Stop.
    kernel::halt();
}

void
kernel::panic(const char* s) noexcept
{
    // Draw the message in hilited red text at the top-left corner of the
    // screen.
    kernel::vga_write(0,0,"Kernel panic");
    kernel::vga_write(0,1,s);
    kernel::serial_write("Kernel panic\n");
    kernel::serial_write(s);
    kernel::serial_write("\n");
    kernel::halt();
}
