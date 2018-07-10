#include "tlb.h"
#include "cpu.h"
#include "interrupts/lapic.h"

#define BATCH_SIZE  64UL

void
kernel::tlb_shootdown()
{
    uint32_t tlb_shootdown_counters[BATCH_SIZE];
    auto cpus  = kernel::cpus.begin();
    size_t rem = kernel::cpus.size();
    while (rem)
    {
        size_t batch_size = min(rem,BATCH_SIZE);

        for (size_t i=0; i<batch_size; ++i)
        {
            tlb_shootdown_counters[i] = cpus[i]->tlb_shootdown_counter;
            kernel::send_tlb_shootdown_ipi(cpus[i]->apic_id);
        }
        for (size_t i=0; i<batch_size; ++i)
        {
            while (tlb_shootdown_counters[i] == cpus[i]->tlb_shootdown_counter)
                ;
        }

        cpus += batch_size;
        rem  -= batch_size;
    }
}
