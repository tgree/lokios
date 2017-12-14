#include "kernel_args.h"
#include "console.h"
#include "cpu.h"
#include "interrupt.h"
#include "kassert.h"
#include "mm/e820.h"
#include "acpi/tables.h"

const kernel::kernel_args* kernel::kargs;

__thread uint64_t tls_signature = 0x135724683579468A;

int
main()
{
    // Initialize the CPU and interrupts.
    kernel::init_main_cpu();
    kernel::init_interrupts();
    kernel::init_acpi_tables(kernel::kargs->e820_base);

    // Banner.
    kernel::vga->printf("Loki is rad\n");
    kernel::kassert(tls_signature == 0x135724683579468A);

    // Test exceptions.
    try
    {
        throw kernel::vga;
        kernel::panic("throw failed");
    }
    catch (int)
    {
        kernel::panic("caught int");
    }
    catch (kernel::console* c)
    {
        kernel::vga->printf("caught console*\n");
    }
    catch (...)
    {
        kernel::panic("caught ...");
    }
}
