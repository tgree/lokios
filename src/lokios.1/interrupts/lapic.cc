#include "lapic.h"
#include "interrupt.h"
#include "kernel/console.h"
#include "kernel/x86.h"
#include "kernel/pmtimer.h"
#include "acpi/tables.h"
#include "mm/mm.h"

using namespace kernel;
using kernel::console::printf;

static lapic_registers* lapic;

static kernel::tls_tcb*
lapic_spurious_interrupt(uint64_t selector, uint64_t error_code)
{
    // Do nothing.  This is triggered if the LAPIC signalled an INTR to the
    // CPU, the CPU sent back an INTA (acknowledge) after which the LAPIC is
    // supposed to then invoke the interrupt vector.  If the vector has been
    // masked between the INTR and INTA then the LAPIC invokes the spurious
    // vector instead which should be a no-op.  Presumably this is required to
    // complete the INTR/INTA/... bus transactions properly.
    return get_current_tcb();
}

static volatile bool lapic_test_passed = false;
static kernel::tls_tcb*
lapic_interrupt_self_test(uint64_t selector, uint64_t error_code)
{
    lapic_test_passed = true;
    lapic->eoi        = 0;
    return get_current_tcb();
}

void
kernel::init_lapic()
{
    const sdt_header* h = find_acpi_table(MADT_SIG);
    kassert(h != NULL);

    const madt_table* madt   = (const madt_table*)h;
    uint64_t local_apic_addr = madt->local_apic_addr;
    for (auto& r : *madt)
    {
        if (r.type == MADT_TYPE_LAPIC_ADDRESS_OVERRIDE)
        {
            local_apic_addr = r.type5.local_apic_addr;
            break;
        }
    }

    printf("LOCAL_APIC_ADDR 0x%016lX\n",local_apic_addr);
    printf("IA32_APIC_BASE  0x%016lX\n",rdmsr(0x1B));

    // Map the local APIC.  Since the local APIC is at the same physical
    // address in all CPUs, this maps it for them all.
    lapic = (lapic_registers*)iomap(local_apic_addr);
    printf("LAPIC: apic_id 0x%08X apic_version 0x%08X tp 0%08X\n",
           (uint32_t)lapic->apic_id,(uint32_t)lapic->apic_version,
           (uint32_t)lapic->task_priority);

    // Mask all local APIC interrupts.
    uint8_t max_lvt = (lapic->apic_version >> 16);
    switch (max_lvt)
    {
        case 6:
            lapic->lvt_cmci             |= 0x00010000;
        case 5:
            lapic->lvt_thermal_sensor   |= 0x00010000;
        case 4:
            lapic->lvt_perfmon_counters |= 0x00010000;
        case 3:
            lapic->lvt_timer            |= 0x00010000;
            lapic->lvt_lint[0]          |= 0x00010000;
            lapic->lvt_lint[1]          |= 0x00010000;
            lapic->lvt_error            |= 0x00010000;
        break;

        default:
            kernel::panic("Can't handle this max_lvt value!");
        break;
    }

    // Set the spurious interrupt vector to be 127.  We need the low-order 4
    // bits to be set.
    register_handler(INTN_LAPIC_SPURIOUS,lapic_spurious_interrupt);
    lapic->spurious_interrupt_vector = 0x00000100 | INTN_LAPIC_SPURIOUS;

    // Set up a local APIC test handler on vector 125.  This will be used to
    // test sending an interrupt to ourselves via the LAPIC.
    register_handler(INTN_LAPIC_SELFTEST,lapic_interrupt_self_test);
}

void
kernel::test_lapic()
{
    lapic->eoi = 0;
    for (size_t i=0; i<2; ++i)
    {
        printf("LAPIC IPI self-test %zu\n",i);
        kassert(lapic_test_passed == false);
        lapic->icr[1] = 0x00000000;
        lapic->icr[0] = 0x00044000 | 125;
        pmtimer::wait_us(100);
        kassert(lapic_test_passed == true);
        lapic_test_passed = false;
    }
}
