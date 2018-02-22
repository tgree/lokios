#include "cmos.h"
#include "console.h"

using kernel::console::printf;

static bool bcd;
static bool twenty_four_hour_mode;

static uint8_t
read_cmos_reg(uint8_t reg)
{
    outb(reg,0x70);
    return inb(0x71);
}

static uint8_t
read_cmos_date_reg(uint8_t reg)
{
    uint8_t val = read_cmos_reg(reg);
    if (!bcd)
        return val;
    return ((val >> 4) & 0x0F)*10 + (val & 0x0F);
}

static uint8_t
read_cmos_hour_reg()
{
    uint8_t val = read_cmos_reg(4);
    uint8_t offset = 0;
    if (!twenty_four_hour_mode)
    {
        if (val & 0x80)
            offset = 12;
        val &= ~0x80;
    }
    if (!bcd)
        return offset + val;
    return offset + ((val >> 4) & 0x0F)*10 + (val & 0x0F);
}

static uint8_t
read_status_a()
{
    return read_cmos_reg(0x0A);
}

static uint8_t
read_status_b()
{
    return read_cmos_reg(0x0B);
}

static uint16_t
read_year()
{
    return 2018;
}

static kernel::date_time
read_date_time()
{
    // Wait for any in-progress update to clear.
    while (read_status_a() & (1<<7))
        ;
    return kernel::date_time(read_year(),read_cmos_date_reg(8),
                             read_cmos_date_reg(7),read_cmos_hour_reg(),
                             read_cmos_date_reg(2),read_cmos_date_reg(0));
}

kernel::date_time
kernel::read_cmos_date_time()
{
    for (;;)
    {
        date_time dt1 = read_date_time();
        date_time dt2 = read_date_time();
        if (dt1 == dt2)
            return dt1;
    }
}

void
kernel::init_cmos()
{
    kassert(date_time(1,2,3,4,5,6) < date_time(1,2,3,4,5,7));

    uint8_t sb = read_status_b();
    bcd = !(sb & (1 << 2));
    twenty_four_hour_mode = (sb & (1<<1));

    auto dt = read_cmos_date_time();
    printf("Current date: %04lu/%02lu/%02lu %02lu:%02lu:%02lu\n",
           dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second);
}
