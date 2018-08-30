#include "pmtimer.h"
#include "kernel/console.h"
#include "acpi/tables.h"
#include "hdr/x86.h"

#define PMTIMER_FREQ    3579545UL
#define MAX_TICKS       0x00800000

using kernel::_kassert;
using kernel::console::printf;

static uint16_t pmtimer_ioaddr;

static uint32_t
read_pmtimer()
{
    return inl(pmtimer_ioaddr);
}

static void
wait_ticks(uint32_t ticks)
{
    kassert(ticks <= MAX_TICKS);
    uint32_t start_ticks = read_pmtimer();
    while (read_pmtimer() - start_ticks < ticks)
        ;
}

void
kernel::pmtimer::wait_us(uint64_t us)
{
    uint64_t ticks = PMTIMER_FREQ*us/1000000UL;
    while (ticks > MAX_TICKS)
    {
        wait_ticks(MAX_TICKS);
        ticks -= MAX_TICKS;
    }
    if (ticks)
        wait_ticks(ticks);
}

void
kernel::pmtimer::init()
{
    auto* fadt = (const fadt_table*)find_acpi_table(FADT_SIG);
    kassert(fadt != NULL);
    if (offsetof(fadt_table,x_pm_tmr_blk) < fadt->hdr.length)
    {
        printf("PM TIMER: %02X %02X %02X %02X %016lX\n",
               fadt->x_pm_tmr_blk.addr_space_id,
               fadt->x_pm_tmr_blk.register_bit_width,
               fadt->x_pm_tmr_blk.register_bit_offset,
               fadt->x_pm_tmr_blk.access_size,
               fadt->x_pm_tmr_blk.addr);
        kassert(fadt->x_pm_tmr_blk.addr_space_id == 1);
        kassert(fadt->x_pm_tmr_blk.register_bit_width == 32);
        kassert(fadt->x_pm_tmr_blk.register_bit_offset == 0);
        kassert(fadt->x_pm_tmr_blk.access_size == 3 ||
                fadt->x_pm_tmr_blk.access_size == 0);
        kassert(fadt->x_pm_tmr_blk.addr < 0x00010000);
        pmtimer_ioaddr = fadt->x_pm_tmr_blk.addr;
    }
    else
    {
        printf("PM TIMER: %08X\n",fadt->pm_tmr_blk);
        kassert(fadt->pm_tmr_blk < 0x00010000);
        pmtimer_ioaddr = fadt->pm_tmr_blk;
    }
}
