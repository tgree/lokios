#include "kernel_args.h"
#include "vga.h"
#include "serial.h"
#include "task.h"
#include "cpu.h"
#include "platform/platform.h"
#include "mm/mm.h"
#include <stddef.h>

using kernel::console::printf;

extern void (*__preinit_array_start[])();
extern void (*__preinit_array_end[])();
extern void (*__init_array_start[])();
extern void (*__init_array_end[])();
extern void (*__fini_array_start[])();
extern void (*__fini_array_end[])();
extern char _eh_frame_begin[];

extern "C" void _init();
extern "C" void init();
extern "C" void __register_frame(char*);

extern int main();

static void
main_bounce()
{
    main();
    printf("Kernel exiting successfully.\n");
    kernel::exit_guest(1);
}

static void
init_globals()
{
    // Do the __preinit array.
    size_t n = __preinit_array_end - __preinit_array_start;
    for (size_t i=0; i<n; ++i)
        __preinit_array_start[i]();

    // Do _init.  I'm not sure exactly what this calls - seemingly nothing.
    _init();

    // Do the __init array.  This calls all the global constructors and stuff
    // in the libsupc++ library which does malloc().
    n = __init_array_end - __init_array_start;
    for (size_t i=0; i<n; ++i)
        __init_array_start[i]();
}

void
init()
{
    // Initialize the console as early as we can.
    kernel::init_vga_console();
    kernel::init_serial_console(0x3F8,kernel::N81_115200);

    // Initialize the memory map.
    kernel::init_mm(kernel::kargs->e820_base);

    // Register exception handling support.  This is going to require the use
    // of malloc() which is why we can't set up exceptions before init_mm() is
    // called.
    __register_frame(_eh_frame_begin);

    // Initialize globals.
    init_globals();

    // Initialize the platform.
    kernel::init_platform();

    // Create the kernel task.  This holds the kernel memory map and all the
    // kernel threads.
    kernel::init_kernel_task();

    // Spawn the main thread in the kernel task.  This will create the thread
    // but we won't start running it yet since no CPUs are available to the
    // scheduler yet.
    kernel::kernel_task->spawn_thread(main_bounce);

    // Initialize the boot CPU and make it available to the scheduler.  The
    // scheduler will take ownership of the CPU; this should never return.
    kernel::init_this_cpu();
}
