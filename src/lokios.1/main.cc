#include "kernel_args.h"
#include "console.h"
#include "pmtimer.h"
#include "cpu.h"
#include "cxx_exception.h"
#include "kassert.h"
#include "acpi/tables.h"
#include "interrupts/interrupt.h"
#include "interrupts/routing.h"
#include "interrupts/lapic.h"
#include "pci/pci.h"
#include "platform/platform.h"
#include <typeinfo>

using kernel::console::printf;

const kernel::kernel_args* kernel::kargs;

__thread uint64_t tls_signature = 0x135724683579468A;

void
kernel_main()
{
    // Initialize the CPU and interrupts.
    kernel::init_acpi_tables(kernel::kargs->e820_base);
    kernel::pmtimer::init();
    kernel::init_interrupts();
    kernel::init_irq_routing();
    kernel::init_lapic_periodic();
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

    printf("Kernel exiting successfully.\n");
    kernel::exit_guest(1);
}
