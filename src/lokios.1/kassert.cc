#include "kassert.h"
#include "kernel_args.h"
#include "vga.h"
#include "serial.h"
#include "k++/string_stream.h"
#include "platform/platform.h"

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
    kernel::exit_guest(2);
}

void
kernel::panic(const char* s, const char* f, unsigned int l) noexcept
{
    // Convert the line number to a string.
    fixed_string_stream<20> fss;
    fss.printf(":%u: ",l);

    // Draw the message in hilited red text at the top-left corner of the
    // screen.
    size_t x = 0;
    kernel::vga_write(0,0,"Kernel panic");
    x += kernel::vga_write(x,1,f);
    kernel::vga_write(x,1,fss);
    kernel::vga_write(0,2,s);

    // Emit the message to the serial port.
    kernel::serial_write("Kernel panic\n");
    kernel::serial_write(f);
    kernel::serial_write(fss);
    kernel::serial_write("\n");
    kernel::serial_write(s);
    kernel::serial_write("\n");

    // If we're in a VM, kill the guest.
    kernel::exit_guest(3);
}
