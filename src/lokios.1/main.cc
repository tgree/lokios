#include "kernel_args.h"
#include "console.h"
#include "cpu.h"
#include "task.h"
#include "cxx_exception.h"
#include "kassert.h"
#include "schedule.h"
#include "kbacktrace.h"
#include "pci/pci.h"
#include "platform/platform.h"
#include "acpi/tables.h"
#include <typeinfo>

#define TICKER_10MS_TICKS   9999

using kernel::console::printf;

__thread uint64_t tls_signature = 0x135724683579468A;

static void
kernel_hello(kernel::work_entry* wqe)
{
    kernel::free_wqe(wqe);
    printf("Hello from CPU%zu.\n",kernel::get_current_cpu()->cpu_number);
}

static kernel::timer_entry one_sec_wqe;
static void
kernel_ticker(kernel::timer_entry* wqe)
{
    kernel::get_current_cpu()->scheduler.schedule_timer(wqe,TICKER_10MS_TICKS);
    printf("Free pages: %zu  PT Used Pages: %zu\n",
           kernel::page_count_free(),kernel::kernel_task->pt.page_count);
}

static kernel::timer_entry ten_sec_wqe;
static void
integration_test_timeout(kernel::timer_entry* wqe)
{
    kernel::panic("integration test timeout");
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

    // Print a backtrace.
    kernel::backtrace(NULL,0);

    // Set up a repeating ticker.
    one_sec_wqe.fn = kernel_ticker;
    kernel::get_current_cpu()->scheduler.schedule_timer(
            &one_sec_wqe,TICKER_10MS_TICKS);

    // If there is an 'iTST' ACPI table, this indicates we are running on qemu
    // in integration-test mode and should start a 10-second timeout after which
    // we decalre the integration test to have failed.
    if (kernel::find_acpi_table('TSTi'))
    {
        ten_sec_wqe.fn = integration_test_timeout;
        kernel::get_current_cpu()->scheduler.schedule_timer(
                &ten_sec_wqe,1000);
    }
}
