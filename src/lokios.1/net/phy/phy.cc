#include "phy.h"
#include "kernel/console.h"
#include "kernel/pmtimer.h"
#include "kernel/x86.h"

#define intf_dbg(fmt,...) \
    kernel::console::printf("eth%zu: " fmt,intf->id,##__VA_ARGS__)

static kernel::klist<eth::phy_driver> drivers;
extern uint64_t tsc_freq;

eth::phy_driver::phy_driver(const char* name):
    name(name)
{
    drivers.push_back(&link);
}

eth::phy*
eth::phy_driver::probe(eth::interface* intf)
{
    uint32_t phy_id_msb = intf->phy_read_16(2);
    uint32_t phy_id_lsb = intf->phy_read_16(3);
    uint32_t phy_id     = ((phy_id_msb << 10) & 0x03FFFC00) |
                          ((phy_id_lsb << 16) & 0xFC000000) |
                          ((phy_id_lsb <<  0) & 0x000003FF);

    phy_driver* driver = NULL;
    uint64_t best_score = 0;
    for (auto& drv : klist_elems(drivers,link))
    {
        uint64_t score = drv.score(intf,phy_id);
        if (score > best_score)
        {
            driver     = &drv;
            best_score = score;
        }
    }

    if (!driver)
    {
        kernel::console::printf("eth%zu: no driver for phy with id 0x%08X\n",
                                intf->id,phy_id);
        return NULL;
    }

    eth::phy* phy = driver->alloc(intf,phy_id);
    driver->phys.push_back(&phy->link);
    return phy;
}

eth::phy::phy(eth::interface* intf, eth::phy_driver* owner):
    intf(intf),
    owner(owner)
{
}

eth::phy::~phy()
{
}

void
eth::phy::dump_regs()
{
    uint16_t regs[32];
    for (size_t i=0; i<32; ++i)
        regs[i] = phy_read_16(i);

    intf_dbg("             0    1    2    3    4    5    6    7\n");
    intf_dbg("phy regs: %04X %04X %04X %04X %04X %04X %04X %04X\n",
             regs[0],regs[1],regs[2],regs[3],
             regs[4],regs[5],regs[6],regs[7]);
    intf_dbg("phy regs: %04X %04X %04X %04X %04X %04X %04X %04X\n",
             regs[8],regs[9],regs[10],regs[11],
             regs[12],regs[13],regs[14],regs[15]);
    intf_dbg("phy regs: %04X %04X %04X %04X %04X %04X %04X %04X\n",
             regs[16],regs[17],regs[18],regs[19],
             regs[20],regs[21],regs[22],regs[23]);
    intf_dbg("phy regs: %04X %04X %04X %04X %04X %04X %04X %04X\n",
             regs[24],regs[25],regs[26],regs[27],
             regs[28],regs[29],regs[30],regs[31]);
}

void
eth::phy::reset()
{
    // TODO: Timeouts!  This should really become async.
    phy_write_16(0x8000,0x00);
    while (phy_read_16(0x00) & 0x8000)
        kernel::pmtimer::wait_us(10);
    kernel::pmtimer::wait_us(40);
}

void
eth::phy::start_autonegotiation()
{
    phy_set_16((1<<12) | (1<<9),0x00);
}
