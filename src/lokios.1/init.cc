#include "kernel_args.h"
#include "vga.h"
#include "task.h"
#include "cpu.h"
#include "mm/mm.h"
#include <stddef.h>

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
    // Main screen turn on and never off.
    main();
    kernel::halt();
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

    // Register exception handling support.  This seems to require __init array
    // to have been done first - but then we can't use exceptions in global
    // constructors?
    __register_frame(_eh_frame_begin);
}

void
init()
{
    // Initialize the console as early as we can.
    kernel::init_vga_console();

    // Initialize the memory map.
    kernel::init_mm(kernel::kargs->e820_base);

    // Initialize globals.
    init_globals();

    // Initialize the main CPU.  We need to do this before initializing tasks
    // since this is where the GDT gets re-initialized and we need the GDT for
    // starting the first thread.
    kernel::init_main_cpu();

    // Start the kernel task with a thread that will invoke main().
    kernel::init_kernel_task(main_bounce);
}
