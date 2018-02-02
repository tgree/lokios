#include "kernel_args.h"
#include "console.h"
#include "cpu.h"
#include "cxx_exception.h"
#include "kassert.h"
#include "schedule.h"
#include "pci/pci.h"
#include "platform/platform.h"
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

    printf("Kernel exiting successfully.\n");
    kernel::exit_guest(1);
}
