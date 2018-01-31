#include "lapic.h"
#include "interrupt.h"
#include "routing.h"
#include "kernel/console.h"
#include "kernel/msr.h"
#include "kernel/pmtimer.h"
#include "kernel/schedule.h"
#include "kernel/cpu.h"
#include "acpi/tables.h"
#include "mm/mm.h"

#define LAPIC_CALIBRATE_MS  200

using namespace kernel;
using kernel::console::printf;

static lapic_registers* lapic;
static uint64_t lapic_frequency;
static uint32_t lapic_periodic_value;
kernel::vector<lapic_configuration> kernel::lapic_configs;

static lapic_configuration*
find_lapic_by_acpi_processor_id(uint8_t acpi_processor_id)
{
    for (auto& lac : kernel::lapic_configs)
    {
        if (lac.acpi_processor_id == acpi_processor_id)
            return &lac;
    }
    return NULL;
}

static lapic_configuration*
find_lapic_by_apic_id(uint8_t apic_id)
{
    for (auto& lac : kernel::lapic_configs)
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

static volatile uint64_t jiffies = 0;
static kernel::tls_tcb*
lapic_interrupt_periodic(uint64_t selector, uint64_t error_code)
{
    if (kernel::get_current_cpu() == kernel::cpus[0])
    {
        ++jiffies;
        if ((jiffies % 100) == 0)
            printf("Elapsed: %lu sec\n",jiffies/100);
    }
    lapic->eoi = 0;
    return kernel::schedule_tick();
}

void
kernel::lapic_eoi()
{
    lapic->eoi = 0;
}

static uint64_t
lapic_measure_ticks(uint64_t interval_ms)
{
    // Stop the APIC timer.
    lapic->initial_count = 0;

    // Set the divider to 2.
    lapic->divide_configuration = 0;

    // The timer is already configured for 1-shot mode.  Start the timer and
    // wait 10ms.
    lapic->initial_count = 0xFFFFFFFF;
    kernel::pmtimer::wait_us(interval_ms*1000);
    uint32_t final_count = lapic->current_count;

    // Stop the APIC timer.
    lapic->initial_count = 0;

    return ~final_count;
}

static uint64_t
lapic_calibrate(uint64_t calibration_interval_ms)
{
    // Calculate the frequency (taking into account the divider of 2), rounding
    // to the nearest MHz.
    uint64_t elapsed_ticks = lapic_measure_ticks(calibration_interval_ms);
    uint64_t freq = ((uint64_t)elapsed_ticks*2*1000/calibration_interval_ms);
    return round_to_nearest_multiple(freq,1000000UL);
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
    printf("IA32_APIC_BASE  0x%016lX\n",rdmsr(IA32_APIC_BASE));

    // Map the local APIC.  Since the local APIC is at the same physical
    // address in all CPUs, this maps it for them all.
    lapic = (lapic_registers*)iomap(local_apic_addr);
    printf("LAPIC: apic_id 0x%08X apic_version 0x%08X tp 0%08X\n",
           (uint32_t)lapic->apic_id,(uint32_t)lapic->apic_version,
           (uint32_t)lapic->task_priority);

    // Set the spurious interrupt vector to be 127.  We need the low-order 4
    // bits to be set.
    register_handler(INTN_LAPIC_SPURIOUS,lapic_spurious_interrupt);

    // Set up a local APIC test handler on vector 125.  This will be used to
    // test sending an interrupt to ourselves via the LAPIC.
    register_handler(INTN_LAPIC_SELFTEST,lapic_interrupt_self_test);

    // Set up a periodic handler on vector 124.  This is used to get a periodic
    // 10ms interrupt from the local APIC.
    register_handler(INTN_LAPIC_10MS,lapic_interrupt_periodic);

    // Calibrate the local APIC timer.
    lapic_frequency      = lapic_calibrate(LAPIC_CALIBRATE_MS);
    lapic_periodic_value = lapic_frequency/(2*100);
    printf("LAPIC frequency: %lu Hz\n",lapic_frequency);
    printf("LAPIC periodic:  %u\n",lapic_periodic_value);
}

void
kernel::init_lapic_cpu_interrupts()
{
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
    lapic->spurious_interrupt_vector = 0x00000100 | INTN_LAPIC_SPURIOUS;

    // Save the LAPIC address in KERNEL_GS_BASE.
    wrmsr((uint64_t)lapic,IA32_KERNEL_GS_BASE);
}

void
kernel::lapic_enable_nmi()
{
    uint8_t apic_id = (lapic->apic_id >> 24);
    auto* lac = find_lapic_by_apic_id(apic_id);
    if (!lac || !(lac->flags & LAPIC_FLAG_HAS_LINT_NMI))
        return;

    printf("Enabling NMI for LAPIC 0x%02X\n",apic_id);
    kassert(lac->lint_pin < nelems(lapic->lvt_lint));

    // Figure out the LINT configuration.  When the delivery mode is NMI, the
    // interrupt is always edge-sensitive.  The default polarity should be
    // active-high - at least that's what they use in the Intel Multiprocessor
    // Specification from the '90s so...
    uint32_t val = 0x00000400;
    switch (lac->lint_flags & ACPI_FLAG_POLARITY_MASK)
    {
        case ACPI_FLAG_POLARITY_DEFAULT:
        case ACPI_FLAG_POLARITY_HIGH:    val |= 0x00000000; break;
        case ACPI_FLAG_POLARITY_LOW:     val |= 0x00002000; break;

        default:
            kernel::panic("reserved polarity for nmi?");
        break;
    }
    lapic->lvt_lint[lac->lint_pin] = val;
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
        lapic->icr[0] = 0x00044000 | INTN_LAPIC_SELFTEST;
        pmtimer::wait_us(100);
        kassert(lapic_test_passed == true);
        lapic_test_passed = false;
    }
}

void
kernel::init_lapic_periodic()
{
    // Ensure the timer is stopped with a divisor of 2.
    lapic->initial_count        = 0;
    lapic->divide_configuration = 0;

    // Enable the interrupt.
    lapic->lvt_timer = 0x00020000 | INTN_LAPIC_10MS;

    // Start the timer in one-shot mode.
    lapic->initial_count = lapic_periodic_value;
}

void
kernel::send_init_ipi(uint8_t target_apic_id)
{
    lapic->icr[1] = ((uint32_t)target_apic_id << 24);
    lapic->icr[0] = 0x00004500;
}

void
kernel::send_sipi_ipi(uint8_t target_apic_id)
{
    lapic->icr[1] = ((uint32_t)target_apic_id << 24);
    lapic->icr[0] = 0x00004608;
}

uint8_t
kernel::get_lapic_id()
{
    return (lapic->apic_id >> 24);
}
