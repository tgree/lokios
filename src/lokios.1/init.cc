#include "kernel_args.h"
#include "vga.h"
#include "serial.h"
#include "task.h"
#include "cpu.h"
#include "cmos.h"
#include "pmtimer.h"
#include "schedule.h"
#include "kbacktrace.h"
#include "symtab.h"
#include "platform/platform.h"
#include "mm/mm.h"
#include "mm/sbrk.h"
#include "mm/e820.h"
#include "acpi/tables.h"
#include "interrupts/mp_tables.h"
#include "interrupts/interrupt.h"
#include "interrupts/routing.h"
#include "interrupts/pic.h"
#include "interrupts/ioapic.h"
#include "interrupts/lapic.h"
#include "k++/random.h"
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
extern "C" void init_bsp();
extern "C" void init_ap();
extern "C" void __register_frame(char*);

extern void kernel_main(kernel::work_entry* wqe);

static void init_bsp_stage2();
static void init_ap_stage2();

const kernel::kernel_args* kargs;
static const kernel::e820_map* e820_base;

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
init_bsp()
{
    // Remap kernel args.
    e820_base = (kernel::e820_map*)kernel::phys_to_virt(kargs->e820_base);

    // Initialize the console as early as we can.
    kernel::init_vga_console(kargs->vga_base);
    kernel::init_serial_console(0x3F8,kernel::N81_115200);

    // Initialize the memory map.
    kernel::preinit_mm(e820_base);

    // Register exception handling support.  This is going to require the use
    // of malloc() which is why we can't set up exceptions before preinit_mm()
    // is called.
    __register_frame(_eh_frame_begin);

    // Initialize backtrace support.
    kernel::init_backtrace();

    // Initialize symbol table support.
    kernel::init_symtab();

    // Initialize globals.
    init_globals();
    kernel::freeze_sbrk();

    // Initialize the platform.
    kernel::init_platform();

    // Create the kernel task.  This holds the kernel memory map and all the
    // kernel threads.  The main thread 
    kernel::init_kernel_task();
    kernel::kernel_task->pt.activate();

    // Create the CPU and thread-switch it to the init_bsp_stage2() routine.
    // We need the CPU struct early because it provides the GDT/TSS/IDT that
    // we need for things like interrupts to work.
    kernel::init_this_cpu(init_bsp_stage2);
    kernel::panic("bsp: init_this_cpu() returned!");
}

static void
init_bsp_stage2()
{
    // We now have a working get_current_cpu().  Schedule the main() wqe.
    auto* wqe = kernel::alloc_wqe();
    wqe->fn = kernel_main;
    kernel::get_current_cpu()->scheduler.schedule_local_work(wqe);

    // Init more stuff.
    kernel::init_mm(e820_base);
    kernel::init_acpi_tables(e820_base);
    kernel::init_mp_tables();
    kernel::init_cmos();
    kernel::random_seed(kernel::read_cmos_date_time().val);
    kernel::pmtimer::init();
    kernel::init_interrupts();
    kernel::init_pic();
    kernel::init_ioapics();
    kernel::init_lapic();
    kernel::init_irq_routing();

    // Init local CPU stuff.
    kernel::init_lapic_cpu_interrupts();
    kernel::init_cpu_interrupts();
    kernel::lapic_enable_nmi();
    kernel::test_lapic();
    kernel::init_lapic_periodic();
    kernel::register_cpu();
    kernel::get_current_cpu()->scheduler.workloop();
    kernel::panic("bsp: workloop() returned!");
}

void
init_ap()
{
    kernel::kernel_task->pt.activate();
    kernel::init_this_cpu(init_ap_stage2);
    kernel::panic("ap: init_this_cpu() returned!");
}

static void
init_ap_stage2()
{
    kernel::init_lapic_cpu_interrupts();
    kernel::init_cpu_interrupts();
    kernel::lapic_enable_nmi();
    kernel::test_lapic();
    kernel::init_lapic_periodic();
    kernel::register_cpu();
    kernel::get_current_cpu()->scheduler.workloop();
    kernel::panic("ap: workloop() returned!");
}
