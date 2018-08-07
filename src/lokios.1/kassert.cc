#include "kassert.h"
#include "kernel_args.h"
#include "vga.h"
#include "serial.h"
#include "console.h"
#include "kbacktrace.h"
#include "k++/string_stream.h"
#include "platform/platform.h"

using kernel::console::printf;

void
kernel::halt() noexcept
{
    vga_write(73,24,"Halted");
    serial_write("Halted\n");
    for (;;)
        asm ("hlt;");
}

extern "C" void
abort()
{
    // Do a backtrace and re-print the ABORT message in case the backtrace
    // went off the screen.
    kernel::vga_set_colors(0x4F00);
    printf("ABORT\n");
    kernel::backtrace();
    printf("ABORT\n");

    // Stop.
    kernel::exit_guest(2);
}

void
kernel::panic(const char* s, const char* f, unsigned int l) noexcept
{
    // Print the panic message, do a backtrace and re-print the panic message
    // in case it scrolled off the screen.
    kernel::vga_set_colors(0x4F00);
    printf("PANIC:%s:%u: %s\n",f,l,s);
    kernel::backtrace();
    printf("PANIC:%s:%u: %s\n",f,l,s);

    // If we're in a VM, kill the guest.
    kernel::exit_guest(3);
}
