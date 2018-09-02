#include "phy.h"
#include "net/eth/interface.h"
#include "kern/console.h"
#include "kern/cpu.h"

#define intf_dbg(fmt,...) \
    kernel::console::printf("eth%zu: " fmt,intf->id,##__VA_ARGS__)

using kernel::_kassert;

static kernel::klist<eth::phy_driver> drivers;

static eth::phy*
alloc_phy(eth::interface* intf, uint32_t phy_id)
{
    eth::phy_driver* driver = NULL;
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

struct phy_prober : public eth::phy_state_machine
{
    enum
    {
        WAIT_READ_ID_MSB,
        WAIT_READ_ID_LSB,
    } state;
    uint32_t phy_id;

    void handle_phy_success(kernel::wqe* wqe)
    {
        switch (state)
        {
            case WAIT_READ_ID_MSB:
                phy_id = ((wqe->args[2] << 10) & 0x03FFFC00);
                state = WAIT_READ_ID_LSB;
                intf->issue_phy_read_16(3,&phy_wqe);
            break;

            case WAIT_READ_ID_LSB:
                phy_id |= ((wqe->args[2] << 16) & 0xFC000000) |
                          ((wqe->args[2] <<  0) & 0x000003FF);
                intf->phy = alloc_phy(intf,phy_id);
                complete_and_delete();
            break;
        }
    }

    phy_prober(eth::interface* intf, kernel::wqe* cqe):
        phy_state_machine(intf,cqe),
        state(WAIT_READ_ID_MSB)
    {
        intf->issue_phy_read_16(2,&phy_wqe);
    }
};

struct phy_resetter : public eth::phy_state_machine
{
    enum
    {
        WAIT_RESET_WRITE_DONE,
        WAIT_RESET_READ_DONE,
        WAIT_RESET_TIMEOUT,
    } state;
    kernel::tqe timer_wqe;
    size_t      timer_retries;

    void handle_phy_success(kernel::wqe* wqe)
    {
        switch (state)
        {
            case WAIT_RESET_WRITE_DONE:
                intf->issue_phy_read_16(0,&phy_wqe);
                state = WAIT_RESET_READ_DONE;
            break;

            case WAIT_RESET_READ_DONE:
                if (!(wqe->args[2] & 0x8000))
                    complete_and_delete();
                else if (--timer_retries)
                {
                    kernel::cpu::schedule_timer(&timer_wqe,1);
                    state = WAIT_RESET_TIMEOUT;
                }
                else
                {
                    cqe->args[1] = -1;
                    complete_and_delete();
                }
            break;

            case WAIT_RESET_TIMEOUT:
                kernel::panic("unexpected PHY success while waiting on timer");
            break;
        }
    }

    void handle_timeout(kernel::tqe* wqe)
    {
        kassert(state == WAIT_RESET_TIMEOUT);
        intf->issue_phy_read_16(0,&phy_wqe);
        state = WAIT_RESET_READ_DONE;
    }

    phy_resetter(eth::interface* intf, kernel::wqe* cqe):
        phy_state_machine(intf,cqe),
        state(WAIT_RESET_WRITE_DONE)
    {
        timer_wqe.fn      = timer_delegate(handle_timeout);
        timer_wqe.args[0] = (uintptr_t)this;
        timer_retries     = 10;
        intf->issue_phy_write_16(0x8000,0,&phy_wqe);
    }
};

struct phy_start_autonegotiator : public eth::phy_state_machine
{
    enum
    {
        WAIT_READ_MII_CONTROL,
        WAIT_WRITE_MII_CONTROL,
    } state;

    void handle_phy_success(kernel::wqe* wqe)
    {
        switch (state)
        {
            case WAIT_READ_MII_CONTROL:
                intf->issue_phy_write_16(wqe->args[2] | (1<<12) | (1<<9),0,wqe);
                state = WAIT_WRITE_MII_CONTROL;
            break;

            case WAIT_WRITE_MII_CONTROL:
                complete_and_delete();
            break;
        }
    }

    phy_start_autonegotiator(eth::interface* intf, kernel::wqe* cqe):
        phy_state_machine(intf,cqe),
        state(WAIT_READ_MII_CONTROL)
    {
        intf->issue_phy_read_16(0,&phy_wqe);
    }
};

eth::phy_driver::phy_driver(const char* name):
    name(name)
{
    drivers.push_back(&link);
}

void
eth::phy_driver::issue_probe(eth::interface* intf, kernel::wqe* cqe)
{
    new phy_prober(intf,cqe);
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
eth::phy::issue_reset(kernel::wqe* cqe)
{
    new phy_resetter(intf,cqe);
}

void
eth::phy::issue_start_autonegotiation(kernel::wqe* cqe)
{
    new phy_start_autonegotiator(intf,cqe);
}
