#include "kernel_args.h"
#include "console.h"
#include "cpu.h"
#include "cxx_exception.h"
#include "kassert.h"
#include "schedule.h"
#include "pci/pci.h"
#include "platform/platform.h"
#include "acpi/tables.h"
#include <typeinfo>

using kernel::console::printf;

const kernel::kernel_args* kernel::kargs;

__thread uint64_t tls_signature = 0x135724683579468A;

static void
kernel_hello(kernel::work_entry* wqe)
{
    kernel::free_wqe(wqe);
    printf("Hello from CPU%zu.\n",kernel::get_current_cpu()->cpu_number);
}

static kernel::work_entry one_sec_wqe;
static uint64_t sec_ticks = 0;
static void
kernel_one_sec_tick(kernel::work_entry* wqe)
{
    kernel::get_current_cpu()->scheduler.schedule_deferred_local_work(wqe,99);
    printf("Elapsed: %lu secs  Free pages: %zu\n",
           ++sec_ticks,kernel::page_count_free());
}

void
kernel_main(kernel::work_entry* wqe)
{
    // Free the wqe.
    kernel::free_wqe(wqe);

    // Initialize the CPU and interrupts.
    kernel::init_ap_cpus();
    kernel::pci::init_pci();

    // Banner.
    printf("Loki is rad\n");
    kernel::kassert(tls_signature == 0x135724683579468A);

    // Test exceptions.
    try
    {
        kernel::throw_test_exception();
        kernel::panic("throw failed");
    }
    catch (int)
    {
        kernel::panic("caught int");
    }
    catch (kernel::exception& e)
    {
        printf("caught '%s': %s\n",typeid(e).name(),e.c_str());
    }
    catch (...)
    {
        kernel::panic("caught ...");
    }

    for (auto* c : kernel::cpus)
    {
        auto* wqe = kernel::alloc_wqe();
        wqe->fn   = kernel_hello;
        c->scheduler.schedule_remote_work(wqe);
    }

    // Set up a 1-second ticker.
    one_sec_wqe.fn = kernel_one_sec_tick;
    kernel::get_current_cpu()->scheduler.schedule_deferred_local_work(
            &one_sec_wqe,99);

    // If there is an 'iTST' ACPI table, this indicates we are running on qemu
    // in integration-tes mode and should just exit instead of spinning in the
    // scheduler forever.
    if (kernel::find_acpi_table('TSTi'))
    {
        printf("Kernel exiting successfully.\n");
        kernel::exit_guest(1);
    }
}
