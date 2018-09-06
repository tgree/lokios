#include "kernel_args.h"
#include "console.h"
#include "cpu.h"
#include "task.h"
#include "cxx_exception.h"
#include "kassert.h"
#include "schedule.h"
#include "pci/pci.h"
#include "platform/platform.h"
#include "acpi/tables.h"
#include "net/net.h"
#include <typeinfo>

#define MEM_INFO_TICKS  9999

using kernel::console::printf;

__thread uint64_t tls_signature = 0x135724683579468A;

static void
kernel_hello(kernel::wqe* wqe)
{
    kernel::free_wqe(wqe);
    printf("Hello from CPU%zu.\n",kernel::get_current_cpu()->cpu_number);
}

static kernel::tqe mem_info_wqe;
static void
kernel_ticker(kernel::tqe* wqe)
{
    kernel::get_current_cpu()->scheduler.schedule_timer(wqe,MEM_INFO_TICKS);
    printf("Free pages: %zu  PT Used Pages: %zu\n",
           kernel::page_count_free(),kernel::kernel_task->pt.page_count);
}

static kernel::tqe ten_sec_wqe;
static void
integration_test_timeout(kernel::tqe* wqe)
{
    kernel::panic("integration test timeout");
}

void
kernel_main(kernel::wqe* wqe)
{
    // Free the wqe.
    kernel::free_wqe(wqe);

    // Initialize the CPU and interrupts.
    kernel::init_ap_cpus();
    kernel::pci::init_pci();
    net::init_net();

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

    // Set up a repeating ticker.
    mem_info_wqe.fn = kernel_ticker;
    kernel::cpu::schedule_timer(&mem_info_wqe,MEM_INFO_TICKS);

    // If there is an 'iTST' ACPI table, this indicates we are running on qemu
    // in integration-test mode and should start a 10-second timeout after which
    // we declare the integration test to have failed.
    if (kernel::find_acpi_table('TSTi'))
    {
        ten_sec_wqe.fn = integration_test_timeout;
        kernel::cpu::schedule_timer_sec(&ten_sec_wqe,10);
    }
}
