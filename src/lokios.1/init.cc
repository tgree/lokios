#include "kernel_args.h"
#include "console.h"
#include "page.h"
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

void
init()
{
    // Initialize the console as early as we can.
    kernel::init_console();

    // Initialize the free page list with whatever the bootloader has mapped
    // for us.
    uintptr_t top_addr = (kernel::kargs->highest_pte_val & PAGE_PFN_MASK) +
                         2*1024*1024;
    kernel::page_preinit(kernel::kargs->e820_base,top_addr);

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
