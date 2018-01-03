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
static kernel::vector<lapic_configuration> lapic_configs;

static lapic_configuration*
find_lapic_by_acpi_processor_id(uint8_t acpi_processor_id)
{
    for (auto& lac : lapic_configs)
    {
        if (lac.acpi_processor_id == acpi_processor_id)
            return &lac;
    }
    return NULL;
}

__UNUSED__ static lapic_configuration*
find_lapic_by_apic_id(uint8_t apic_id)
{
    for (auto& lac : lapic_configs)
    {
        if (lac.apic_id == apic_id)
            return &lac;
    }
    return NULL;
}

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

    // First pass: find the local APIC address and populate the APIC list.
    uint64_t local_apic_addr = madt->local_apic_addr;
    for (auto& r : *madt)
    {
        if (r.type == MADT_TYPE_LAPIC_ADDRESS_OVERRIDE)
            local_apic_addr = r.type5.local_apic_addr;
        else if (r.type == MADT_TYPE_LAPIC)
        {
            printf("0: ACPI_PID %u APIC_ID %u FL 0x%08X\n",
                   r.type0.acpi_processor_id,r.type0.apic_id,r.type0.flags);

            // Ignore disabled LAPICs.
            if (!(r.type0.flags & (1<<0)))
                continue;

            lapic_configs.emplace_back(
                lapic_configuration{r.type0.acpi_processor_id,
                                    r.type0.apic_id,0,0,0});
        }
    }

    // Second pass: parse NMI information into the local APIC config.
    for (auto& r : *madt)
    {
        if (r.type != MADT_TYPE_LAPIC_NMI)
            continue;

        printf("4: CPU 0x%02X FL 0x%04X LINT %u\n",
               r.type4.processor,r.type4.flags,r.type4.lint_num);

        if (r.type4.processor == 0xFF)
        {
            for (auto& lac : lapic_configs)
            {
                if (lac.flags & LAPIC_FLAG_HAS_LINT_NMI)
                    continue;

                lac.flags     |= LAPIC_FLAG_HAS_LINT_NMI;
                lac.lint_pin   = r.type4.lint_num;
                lac.lint_flags = r.type4.flags;
            }
        }
        else
        {
            auto* lac = find_lapic_by_acpi_processor_id(r.type4.processor);
            if (!lac || (lac->flags & LAPIC_FLAG_HAS_LINT_NMI))
                continue;

            lac->flags     |= LAPIC_FLAG_HAS_LINT_NMI;
            lac->lint_pin   = r.type4.lint_num;
            lac->lint_flags = r.type4.flags;
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
