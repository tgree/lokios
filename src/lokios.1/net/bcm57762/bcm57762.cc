/*
 * Driver for the BCM 57762 chip found on the Thunderbolt Ethernet adapter.
 * This is one of the chips in the "Tigon3" series, supported by the Linux tg3
 * kernel module.
 *
 * References:
 * https://docs.broadcom.com/docs-and-downloads/collateral/pg/57785-PG105-R.pdf
 *
 * A note on endianness:
 *
 *  This chip is driven by an internal big-endian RISC processor.  It's all
 *  kind of a mess, but this is what you gotta know:
 *
 *    1. Register accesses are all little-endian over the PCI bus.  Don't do
 *       any swapping.
 *    2. For MMIO into the RISC RAM, life is kind of a disaster.  The RISC
 *       RAM maps to the PCI bus in 64-bit quantities (even though we can only
 *       do 32-bit accesses).  The PCI bus samples the low-order bytes for sub-
 *       64-bit operations, but these map to the RISC RAM high-order bytes:
 *
 *          Addr:    0  1  2  3  4  5  6  7
 *          Data:   11 22 33 44 55 66 77 88
 *
 *      So, if we ask for the byte at address 0 (0x11), the controller will
 *      drive the 64-bit quantity, we'll sample the low-order byte and get
 *      0x88 back.  YUCK!  Now, we can enable word-swapping in the chip which
 *      will help a bit:
 *
 *          Addr:    4  5  6  7  0  1  2  3
 *          Data:   55 66 77 88 11 22 33 44
 *
 *      This is closer because now we are at least getting the right 32-bit
 *      word out (and we can now access the first 4 bytes in the memory window
 *      which were inaccessible since 64-bit accesses don't work).  From there
 *      we can make everything work by munging and swapping.
 */
#include "bcm57762.h"
#include "../phy/phy.h"
#include "kernel/console.h"
#include "kernel/cpu.h"
#include "kernel/mm/buddy_allocator.h"
#include <stdlib.h>

#define dev_dbg(fmt,...) \
    kernel::console::printf("bcm57762 %04X:%u.%u: " fmt, \
        bus,(devfn>>3),(devfn&7),##__VA_ARGS__)

#define DEBUG_TRANSITIONS   0
#define TRANSITION(s) \
    do { \
        if (DEBUG_TRANSITIONS) \
            dev_dbg("-> " #s "\n"); \
        state = (s); \
    } while(0)
#define QTRANSITION(s) state = (s)

using kernel::_kassert;

extern uint64_t tsc_freq;

bcm57762::dev::dev(const kernel::pci::dev* pd, const bcm57762::driver* owner):
    kernel::pci::dev(pd,owner),
    phy_cqe(NULL)
{
    dev_dbg("Product ID and ASIC rev: 0x%08X\n",config_read_32(0xFC));

    // Set up work entries.
    timer_wqe.fn          = timer_delegate(handle_timer);
    timer_wqe.args[0]     = (uintptr_t)this;
    phy_probe_wqe.fn      = work_delegate(handle_phy_probe_complete);
    phy_probe_wqe.args[0] = (uintptr_t)this;
    phy_reset_wqe.fn      = work_delegate(handle_phy_reset_complete);
    phy_reset_wqe.args[0] = (uintptr_t)this;
    phy_an_wqe.fn         = work_delegate(handle_phy_start_autoneg_complete);
    phy_an_wqe.args[0]    = (uintptr_t)this;
    phy_lm_wqe.fn         = work_delegate(handle_phy_get_link_mode_complete);
    phy_lm_wqe.args[0]    = (uintptr_t)this;

    // Map everything.
    regs = (registers*)map_bar(0,sizeof(registers));
    mem  = (mem_block*)kernel::buddy_alloc(kernel::ulog2(sizeof(mem_block)));

    // Reset.
    issue_reset();
}

bcm57762::dev::~dev()
{
    kernel::buddy_free(mem,kernel::ulog2(sizeof(mem_block)));
}

void
bcm57762::dev::mem_read(void* _dst, uint32_t addr, size_t len)
{
    uint8_t* dst = (uint8_t*)_dst;
    while (len)
    {
        uint32_t window_addr = (addr & 0x00FF8000);
        size_t window_rem    = (window_addr + 0x8000 - addr);
        size_t rem           = kernel::min(window_rem,len);
        config_write_32(window_addr,0x7C);

        for (size_t i=0; i<rem; ++i)
            *dst++ = regs->window[(addr++ ^ 3) & 0x7FFF];

        len -= rem;
    }
    config_write_32(0,0x7C);
}

void
bcm57762::dev::mem_write(uint32_t addr, const void* _src, size_t len)
{
    const uint8_t* src = (const uint8_t*)_src;
    while (len)
    {
        uint32_t window_addr = (addr & 0x00FF8000);
        size_t window_rem    = (window_addr + 0x8000 - addr);
        size_t rem           = kernel::min(window_rem,len);
        config_write_32(window_addr, 0x7C);

        for (size_t i=0; i<rem; ++i)
            regs->window[(addr++ ^ 3) & 0x7FFF] = *src++;

        len -= rem;
    }
    config_write_32(0,0x7C);
}

uint32_t
bcm57762::dev::reg_read_32(uint32_t offset)
{
    kassert(offset >= 0x0200);
    kassert(offset <= 0x7FFC);
    return *(volatile uint32_t*)&regs->mailbox[offset - 0x0200];
}

void
bcm57762::dev::reg_write_32(uint32_t val, uint32_t offset)
{
    kassert(offset >= 0x0200);
    kassert(offset <= 0x7FFC);
    *(volatile uint32_t*)&regs->mailbox[offset - 0x0200] = val;
}

void
bcm57762::dev::issue_phy_read_16(uint8_t offset, kernel::work_entry* cqe)
{
    kassert(!phy_cqe);
    kassert(offset < 32);

    // Set up the cqe.
    phy_cqe          = cqe;
    phy_cqe->args[1] = 0;

    // Initiate the read op.
    begin_phy_transaction(0x08200000 | (offset << 16));
}

void
bcm57762::dev::issue_phy_write_16(uint16_t v, uint8_t offset,
    kernel::work_entry* cqe)
{
    kassert(!phy_cqe);
    kassert(offset < 32);

    // Set up the cqe.
    phy_cqe          = cqe;
    phy_cqe->args[1] = 1;

    // Initiate the write op.
    begin_phy_transaction(0x04200000 | (offset << 16) | v);
}

void
bcm57762::dev::begin_phy_transaction(uint32_t val)
{
    // Disable autopolling.
    phy_autopoll = (reg_read_32(0x454) & (1<<4));
    if (phy_autopoll)
    {
        reg_clear_32((1<<4),0x454);
        reg_read_32(0x454);
    }

    // Enable PHY completion interrupts.
    reg_set_32(0x00400000,0x408);

    // Start the transaction.
    reg_write_32(val,0x44C);
    reg_write_32(val | (1<<29),0x44C);
}

void
bcm57762::dev::complete_phy_transaction()
{
    kassert(phy_cqe != NULL);

    // ACK the event and disable completion interrupts.
    reg_clear_32(0x00400000,0x408);
    reg_write_32(0x00400000,0x404);

    // Read the status and data.
    uint32_t v = reg_read_32(0x44C);
    kassert((v & (1<<29)) == 0);
    if (v & (1<<28))
        phy_cqe->args[1] = v;
    else
    {
        phy_cqe->args[1] = 0;
        phy_cqe->args[2] = (v & 0xFFFF);
    }

    // Enable autopolling.
    if (phy_autopoll)
    {
        reg_set_32((1<<4),0x454);
        reg_read_32(0x454);
    }

    // Invoke the completion callback.
    kernel::work_entry* cqe = phy_cqe;
    phy_cqe = NULL;
    cqe->fn(cqe);
}

void
bcm57762::dev::issue_reset()
{
    // Misc config register after iPXE
    // = 0xF000029A
    //   - (1<<9) -> enable tagged status mode
    //   - (1<<7) -> enable indirect access
    //   - (1<<4) -> enable PCI State Register r/w
    //   - (1<<3) -> enable endian word swap for target accesses
    //   - (1<<1) -> mask interrupts
    //
    // EMAC mode register after iPXE
    // = 

    // Step 1: enable mem space decode and bus mastering.
    config_write_16(0x0506,0x04);

    // Step 2: disable interrupts.
    config_set_32((1<<1),0x68);

    // TODOs:
    // 1. Enable CQ25155 fix in 0x4900 register bit 2.
    // 2. Possibly set disable_cache_alignment in 0x6C?

    // Configure endian word swapping only.
    //
    //  Bit -> Effect
    //  -------------
    //    2 -> endian byte swap
    //    3 -> endian word swap
    //    6 -> register word swap
    config_set_clear_32((1<<3),(1<<6) | (1<<2),0x68);
    config_read_32(0x68);

    // Step 3: write 'KevT' to device memory offset 0xB50.
    char sig[4] = {'K', 'e', 'v', 'T'};
    mem_write(0xB50,sig,4);

    // Step 4: acquire NVRAM lock by setting REQ1 bit and waiting for WON1.
    reg_write_32((1<<1),0x7020);
    timer_retries = 100;
    kernel::cpu::schedule_timer(&timer_wqe,1);
    TRANSITION(WAIT_RESETTING_NVRAM_LOCK);
}

void
bcm57762::dev::handle_vec0_dsr(kernel::work_entry*)
{
    uint32_t status_word = mem->status_block.status_word;
    uint32_t tag         = mem->status_block.tag;

    mem->status_block.status_word = (status_word & ~(1<<0));

    if (state == WAIT_RESETTING_FORCED_INTERRUPT)
    {
        // Instantiate the eth::interface.
        intf = new bcm57762::interface(this);
        intf->issue_probe_phy(&phy_probe_wqe);
        TRANSITION(WAIT_PHY_PROBE_DONE);
    }

    // Check for PHY completions first so that we transition before checking
    // the link state (which can happen at the same time as the PHY completion).
    if (status_word & (1<<2))
    {
        uint32_t events = reg_read_32(0x404);
        if (events & (1<<22))
        {
            reg_write_32((1<<22),0x404);
            handle_phy_completion();
        }
    }

    // Check the link state.
    if (status_word & (1<<1))
    {
        reg_write_32((1<<12),0x404);
        if (reg_read_32(0x450) & (1<<0))
            handle_link_up();
        else
            handle_link_down();
    }

    reg_write_32((tag << 24),0x204);
    enable_msix_vector(0);
}

void
bcm57762::dev::handle_timer(kernel::timer_entry*)
{
    char sig[4];
    uint32_t high;
    uint32_t low;
    ring_control_block send_rcb;
    ring_control_block recv_return_rcbs[4];

    switch (state)
    {
        case WAIT_RESETTING_NVRAM_LOCK:
            if (!(reg_read_32(0x7020) & (1<<9)))
            {
                if (!--timer_retries)
                {
                    dev_dbg("failed to acquire NVRAM lock");
                    TRANSITION(WAIT_DEAD);
                }
                else
                    kernel::cpu::schedule_timer(&timer_wqe,1);
                return;
            }

            // Step 5: clear fast boot program counter register and enable
            // memory arbiter.
            reg_write_32(0,0x6894);
            reg_set_32((1<<1),0x4000);

            // Step 6: reset the core clocks.
            // Note: the procedure says to set bit 26
            // ("GPHY_Power_Down_Override"), but that bit isn't documented in
            // the register description.
            reg_set_32((1<<29) | (1<<26) | (1<<0),0x6804);

            // Step 7: wait for core clock reset to complete.  We must delay
            // for at least 1 ms.
            kernel::cpu::schedule_timer_ms(&timer_wqe,2);
            TRANSITION(WAIT_RESETTING_CORE_CLOCK);
        break;

        case WAIT_RESETTING_CORE_CLOCK:
            // Step 8: disable interrupts.  Interrupts were reenabled by the
            // reset, so we disable them again.
            config_set_32((1<<1),0x68);

            // Step 9: reenable mem space decode and bus mastering.
            config_write_16(0x0506,0x04);

            // Step 10: reenable the MAC memory arbiter.
            reg_set_32((1<<1),0x4000);

            // Step 11: initialize the miscellaneous host control register.  We
            // set endian word swapping, enable the indirect register pairs and
            // enable the PCI State register.
            config_set_clear_32((1<<3) | (1<<4) | (1<<7),(1<<2) | (1<<6),0x68);

            // Step 12: set Byte_Swap_Non_Frame_Data and Byte_Swap_Data in the
            // general mode control register.
            reg_set_32((1<<1) | (1<<4),0x6800);

            // Step 13: set Word_Swap_Data and Word_Swap_Non_Frame_Data in the
            // general mode control register.
            reg_set_32((1<<2) | (1<<5),0x6800);

            // Step 14: configure port mode for GMII devices.  Unclear if we
            // need this, but it's for GbE and that's what the Thunderbolt
            // Dongle supports, so...
            // TODO: GMII vs MII?
            reg_insert_32(0x2,2,3,0x400);

            // Step 15: poll for bootcode completion.  This should complete
            // within 1s for flash devices and within 10s for SEEPROM devices.
            timer_retries = 1000;
            kernel::cpu::schedule_timer(&timer_wqe,4);
            TRANSITION(WAIT_RESETTING_BOOTCODE_DONE);
        break;

        case WAIT_RESETTING_BOOTCODE_DONE:
            mem_read(sig,0xB50,4);
            if (sig[0] != ~'K' || sig[1] != ~'e' || sig[2] != ~'v' ||
                sig[3] != ~'T')
            {
                if (!--timer_retries)
                {
                    dev_dbg("bootcode failed to complete reset");
                    TRANSITION(WAIT_DEAD);
                }
                else
                    kernel::cpu::schedule_timer(&timer_wqe,1);
                return;
            }

            // Step 16: ERRATA.
            TODO("BCM57762 errata");

            // Step 17: enable tagged status mode (optional).  iPXE had this
            // enabled so we'll do the same.
            config_set_32((1<<9),0x68);

            // Step 18: clear the driver status block memory region.
            memset(&mem->status_block,0,sizeof(mem->status_block));

            // Step 19: configure DMA write water mark in the DMA read/write
            // control register as follows:
            //      PCIe Max Payload Size       DMA watermark bits (19-21)
            //      ---------------------       --------------------------
            //      128 bytes                   0b011 (watermark 128 bytes)
            //      >= 256 bytes                0b111 (watermark 256 bytes)
            if (config_extract_32(5,7,0xB4))
                config_insert_32(0x3,19,21,0x6C);
            else
                config_insert_32(0x7,19,21,0x6C);

            // Step 20: configure DMA byte swapping (optional).  It sounds like
            // this is used for big-endian hosts, so we skip this step.
            // TODO: Revisit this if all the host-side data structures look
            // wonky.

            // Step 21: configure the host-based send ring.  Set the
            // Host_Send_BDs bit in the General Mode Control register.
            memset(mem->send_bds,0,sizeof(mem->send_bds));
            tx_bd_producer_index = 0;
            reg_set_32((1<<17),0x6800);

            // Step 22: indicate driver is ready to receive traffic.  Set the
            // Host_Stack_Up bit in the General Mode Control Register.
            reg_set_32((1<<16),0x6800);

            // Step 23: configure TCP/UDP checksum offloading.  We disable it
            // so that we are consistent with the virtio_net driver for now.
            reg_set_32((1<<20) | (1<<23),0x6800);

            // Step 24: config MAC Mbuf memory pool watermarks.  We use the
            // values recommended in the Broadcom doc.
            reg_write_32(0x0000002A,0x4414);
            reg_write_32(0x000000A0,0x4418);

            // Step 25: configure flow control behavior when RX Mbuf low
            // watermark has been reached.  We use the value recommended in the
            // Broadcom doc.
            reg_insert_32(1,0,15,0x504);

            // Step 26: enable the buffer manager by setting Enable and
            // Attn_Enable bits in the Buffer Manager Mode register.
            reg_set_32((1<<1) | (1<<2),0x4400);

            // Step 27: set the BD ring replenish threshold for the RX producer
            // ring to the recommended value of 0x19.
            reg_write_32(0x19,0x2C18);

            // Step 28, 29, 30: initialize the standard receive buffer ring RCB
            // registers.
            memset(mem->recv_bds,0,sizeof(mem->recv_bds));
            reg_write_32(kernel::virt_to_phys(mem->recv_bds) >> 32,0x2450);
            reg_write_32(kernel::virt_to_phys(mem->recv_bds) >>  0,0x2454);
            reg_write_32((512 << 16) | (4096 << 2),0x2458);
            reg_write_32(0x6000,0x245C);

            // Step 31: initialize the receive BD standard producer index.
            reg_write_32(0,0x26C);

            // Step 32: initialize the Standard Ring Replenish Watermark
            // register to the recommended value of 0x20.
            reg_write_32(0x20,0x2D00);

            // Step 33: initialize the Send Ring Producer index registers
            // (plural?) by zeroing the host index.
            reg_write_32(0,0x304);

            // Step 34: initialize the send rings (plural?) by copying their
            // control blocks into NIC memory.
            send_rcb.host_ring_addr = kernel::virt_to_phys(mem->send_bds);
            send_rcb.flags          = 0;
            send_rcb.max_len        = NELEMS(mem->send_bds);
            send_rcb.nic_ring_addr  = 0x4000;
            mem_write(0x0100,&send_rcb,sizeof(send_rcb));

            // Step 35, 36: initialize the receive return ring RCBs.  We only
            // enable receive return ring 1.
            recv_return_rcbs[0].host_ring_addr =
                kernel::virt_to_phys(mem->return_bds);
            recv_return_rcbs[0].flags          = 0;
            recv_return_rcbs[0].max_len        = NELEMS(mem->return_bds);
            recv_return_rcbs[0].nic_ring_addr  = 0x6000;
            for (size_t i=1; i<4; ++i)
            {
                recv_return_rcbs[i].host_ring_addr = 0;
                recv_return_rcbs[i].flags          = (1<<1);
                recv_return_rcbs[i].max_len        = 0;
                recv_return_rcbs[i].nic_ring_addr  = 0x6000;
            }
            mem_write(0x0200,&recv_return_rcbs,sizeof(recv_return_rcbs));

            // Step 37: initialize the Receive Producer Ring mailbox registers
            // (plural?).
            // TODO: We already initialized this in step 31... WTF?
            reg_write_32(0,0x26C);

            // Step 38: configure the MAC unicast address.  On my Thunderbolt
            // adapter, reading them all out here yields:
            //      0: 40:6C:8F:3E:28:88
            //      1: 00:10:18:00:00:00
            //      2: 00:10:18:00:00:00
            //      3: 00:10:18:00:00:00
            // which also happens to correspond to the correct MAC address as
            // configured by the OS X kernel.  We'll copy MAC0 into MAC1-3 here
            // since that seems to be what the docs recommend.
            high = reg_read_32(0x410);
            low  = reg_read_32(0x414);
            mac = eth::addr{(uint8_t)(high >>  8),
                            (uint8_t)(high >>  0),
                            (uint8_t)(low  >> 24),
                            (uint8_t)(low  >> 16),
                            (uint8_t)(low  >>  8),
                            (uint8_t)(low  >>  0)};
            for (size_t i=1; i<4; ++i)
            {
                reg_write_32((mac[0] << 8) | mac[1],0x410 + 8*i);
                reg_write_32((mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) |
                             mac[5],0x414 + 8*i);
            }

            // Step 39: configure the random backoff seed for transmit.  The
            // recommended value is the 9-bit sum of the MAC address bytes.
            reg_write_32((mac[0] + mac[1] + mac[2] + mac[3] + mac[4] + mac[5])
                         & 0x3FF,0x438);

            // Step 40: configure the MTU.  We set it to 4K since our receive
            // buffers are page-sized.
            reg_write_32(4096,0x43C);

            // Step 41: configure the inter-packet gap to the recommended value
            // of 0x2620.
            reg_write_32(0x2620,0x464);

            // Step 42: configure the default RX return ring for non-matched
            // packets.  We set it to return ring 1.
            reg_insert_32(1,3,5,0x500);

            // Step 43: configure the number of receive lists.  This is totally
            // confusing since it isn't clear how RSS and receive rules
            // interact when selecting a final receive return ring; I'm just
            // going to set it to one list for now.  The doc says Broadcom's
            // driver sets this to 0x181.  In register 0x2018, it looks like
            // there are two bits that allow you to prioritize what receive
            // return ring is selected if an RSS or an "RC rule" match (what's
            // an "RC rule"?  Never defined!), so maybe that's how you control
            // the interaction between the two classification systems.
            reg_write_32(0x111,0x2010);

            // Step 44: configure the Receive List Placement Statistics mask.
            // The Broadcom drivers apparently set this to 0x007BFFFF.
            reg_write_32(0x007BFFFF,0x2018);

            // Step 45: enable RX statistics.
            // TODO: Should we also do something with clearing statistics here?
            reg_write_32(0x00000001,0x2014);

            // Step 46: enable the Send Data Initiator mask.  The doc says to
            // write 0x00FFFFFF to this register, but the doc only defines the
            // first bit as writeable... go figure.
            reg_write_32(0x00FFFFFF,0xC0C);

            // Step 47: enable TX statistics.
            // TODO: Should we also do something with clearing statistics here?
            reg_write_32(0x00000003,0xC08);

            // Step 48: disable the host coalescing engine before configuring
            // its parameters.
            reg_write_32(0,0x3C00);

            // Step 49: poll 20ms for the host coalescing engine to stop.
            timer_retries = 3;
            kernel::cpu::schedule_timer(&timer_wqe,1);
            TRANSITION(WAIT_RESETTING_COALESCING_STOP);
        break;

        case WAIT_RESETTING_COALESCING_STOP:
            if (reg_read_32(0x3C00) & (1<<1))
            {
                if (!--timer_retries)
                {
                    dev_dbg("host coalescing failed to stop");
                    TRANSITION(WAIT_DEAD);
                }
                else
                    kernel::cpu::schedule_timer(&timer_wqe,1);
                return;
            }

            // Step 50: configure the host coalescing tick count.  We'll use
            // the recommended RX value of 0x48 and TX value of 0x14.
            reg_write_32(0x48,0x3C08);
            reg_write_32(0x14,0x3C0C);

            // Step 51: configure the host coalescing BD count.  We'll use the
            // recommended RX value of 0x05 and TX value of 0x35.
            reg_write_32(0x05,0x3C10);
            reg_write_32(0x35,0x3C14);

            // Step 52: configure the max-coalesced frames during interrupt
            // counter.  Once again, we'll use the recommended TX and RX value
            // of 0x05.
            reg_write_32(0x05,0x3C20);
            reg_write_32(0x05,0x3C24);

            // Step 53: initialize host status block address.
            reg_write_32(kernel::virt_to_phys(&mem->status_block) >> 32,0x3C38);
            reg_write_32(kernel::virt_to_phys(&mem->status_block) >>  0,0x3C3C);

            // Step 54: enable the host coalescing engine.
            reg_set_32((1<<1),0x3C00);

            // Step 55: enable the receive BD completion functional block by
            // setting the Enable and Attn_Enable bits in the Receive BD
            // Completion Mode register.
            reg_write_32(0x00000006,0x3000);

            // Step 56: enable the receive list placement functional block by
            // setting the Enable bit in the Receive List Placement Mode
            // register.
            reg_set_32((1<<1),0x2000);

            // Step 57: enable the DMA engines.
            reg_set_32((1<<23) | (1<<22) | (1<<21),0x400);

            // Step 58: enable and clear statistics.
            reg_set_32((1<<15) | (1<<14) | (1<<12) | (1<<11),0x400);

            // Step 59: delay 40 microseconds.
            kernel::cpu::schedule_timer(&timer_wqe,1);
            TRANSITION(WAIT_RESETTING_DMA_READY);
        break;

        case WAIT_RESETTING_DMA_READY:
            // Step 60: configure the General Miscellaneous Local control
            // register.
            reg_set_32((1<<3),0x6808);

            // Step 61: delay 100 microseconds.
            kernel::cpu::schedule_timer(&timer_wqe,1);
            TRANSITION(WAIT_RESETTING_GML_READY);
        break;

        case WAIT_RESETTING_GML_READY:
            // Step 62, 63: configure the Write DMA Mode register.
            reg_write_32((1<<29) | (1<<9) | (1<<8) | (1<<7) | (1<<6) | (1<<5) |
                         (1<<4) | (1<<3) | (1<<2) | (1<<1),0x4C00);

            // Step 64: delay 40 microseconds.
            kernel::cpu::schedule_timer(&timer_wqe,1);
            TRANSITION(WAIT_RESETTING_WDMA_READY);
        break;

        case WAIT_RESETTING_WDMA_READY:
            // Step 65: configure the Read DMA Mode register.
            reg_write_32((1<<9) | (1<<8) | (1<<7) | (1<<6) | (1<<5) | (1<<4) |
                         (1<<3) | (1<<2) | (1<<1),0x4800);

            // Step 66: delay 40 microseconds.
            kernel::cpu::schedule_timer(&timer_wqe,1);
            TRANSITION(WAIT_RESETTING_RDMA_READY);
        break;

        case WAIT_RESETTING_RDMA_READY:
            // Step 67: enable the receive data completion functional block by
            // setting the Enable and Attn_Enable bits in the Receive Data
            // Completion Mode register.
            reg_write_32(0x00000006,0x2800);

            // Step 68: enable the send data completion functional block by
            // setting the Enable bit in the Send Data Completion Mode register.
            reg_write_32((1<<1),0x1000);

            // Step 69: enable the send BD completion functional block by
            // setting the Enable and Attn_Enable bits in the Send BD
            // Completion Mode register.
            reg_write_32(0x00000006,0x1C00);

            // Step 70: enable the Receive BD Initiator functional block by
            // setting the Enable and Receive_BDs_Available_On_Receive_BD_Ring
            // in the Receive BD Initiator Mode register.
            reg_write_32(0x00000006,0x2C00);

            // Step 71: enable the receive data and BD initiator functional
            // block by setting the Enable and Illegal_Return_Ring_Size bits in
            // the Receive Data and Receive BD Initiator Mode register.
            reg_write_32((1<<4) | (1<<1),0x2400);

            // Step 72: enable the send data initiator functional block by
            // setting the Enable bit in the Send Data Initiator Mode register.
            reg_write_32((1<<1),0xC00);

            // Step 73: enable the send BD initiator functional block by
            // setting the Enable and the Attn_Enable bits in the Send BD
            // Initiator Mode register.
            reg_write_32(0x00000006,0x1800);

            // Step 74: enable the send BD selector functional block by setting
            // the Enable and Attn_Enable bits in the Send BD Selector Mode
            // register.
            reg_write_32(0x00000006,0x1400);

            // Step 75: replenish the receive BD producer ring with the receive
            // BDs.
            for (size_t i=0; i<128; ++i)
            {
                void* p = kernel::page_alloc();
                memset(p,0xEE,PAGE_SIZE);
                mem->recv_bds[i].host_addr = kernel::virt_to_phys(p);
                mem->recv_bds[i].len       = PAGE_SIZE;
                mem->recv_bds[i].index     = i;
                mem->recv_bds[i].opaque    = 0x11111111;
            }
            reg_write_32(128,0x26C);
            
            // Step 76: enable the transmit MAC by setting the Enable bit and
            // the Enable_Bad_TxMBUF_Lockup_fix bit in the Transmit MAC Mode
            // register.
            reg_set_32((1<<8) | (1<<1),0x45C);

            // Step 77: delay 100 microseconds.
            kernel::cpu::schedule_timer(&timer_wqe,1);
            TRANSITION(WAIT_RESETTING_TXMAC_READY);
        break;

        case WAIT_RESETTING_TXMAC_READY:
            // Step 78: enable the receive MAC by setting the Enable bit in the
            // Receive MAC Mode register.
            reg_set_32((1<<1),0x468);
            
            // Step 79: delay 10 microseconds.
            kernel::cpu::schedule_timer(&timer_wqe,1);
            TRANSITION(WAIT_RESETTING_RXMAC_READY);
        break;

        case WAIT_RESETTING_RXMAC_READY:
            // Step 80: set up the LED Control register using the recommended
            // value of 0x800.
            reg_write_32(0x00000800,0x40C);

            // Step 81: activate link and enable MAC functional blocks by
            // setting the Link_Status bit in the MII status register.
            reg_write_32(1,0x450);

            // Step 82: optionally (?) disable auto-polling on the management
            // interface.  We don't do this because I can't make any sense of
            // mapping the recommended 0xC0000 value to the register
            // description.
            // TODO: figure this out!

            // Step 83: set the Low Watermark Maximum Receive Frame register to
            // the recommended value of 1.
            // TODO: umm, what about the TXFIFO Almost Empty Threshold field
            // here?  It doesn't sound like something we want to clear, but
            // that's what the doc implies here by ignoring the field
            // completely.
            reg_insert_32(1,0,15,0x504);

            // Step 84: optionally configure PMCSR.
            // TODO: sounds like this is already configured to the correct
            // reset value of 0x00 and only needs to be reconfigured if moving
            // from D3hot/cold.

            // Step 85: "set up the physical layer and restart auto-negotation.
            // For details on PHY auto-negotiation, refer to the applicable PHY
            // data sheet".
            // This happens after we return.

            // Step 86: set up multicast filters.  We set these filters to all-
            // ones so that all multicast and broadcast packets are received.
            // TOOD: Actually populate these according the the CRC32 of the
            // broadcast address.
            reg_write_32(0xFFFFFFFF,0x470);
            reg_write_32(0xFFFFFFFF,0x474);
            reg_write_32(0xFFFFFFFF,0x478);
            reg_write_32(0xFFFFFFFF,0x47C);

            // Our stuff: configure MSI-X.
            alloc_msix_vector(0,work_delegate(handle_vec0_dsr));
            enable_msix_vector(0);

            // Our stuff: clear MAC interrupt status and enable interrupt
            // sources.
            reg_write_32(0x1C401000,0x404);
            reg_write_32(0x00001000,0x408);

            // Our stuff: set Interrupt_On_MAC_Attention in the Mode Control
            // register so we get interrupts when enabled MAC events are
            // triggered.
            reg_set_32((1<<26),0x6800);

            // Force an initial interrupt.
            reg_set_32((1<<3),0x3C00);
            TRANSITION(WAIT_RESETTING_FORCED_INTERRUPT);
        break;

        case WAIT_RESETTING_FORCED_INTERRUPT:
        case WAIT_PHY_PROBE_DONE:
        case WAIT_PHY_RESET_DONE:
        case WAIT_PHY_START_AUTONEG_DONE:
        case WAIT_PHY_LINK_NOTIFICATION:
        case READY_LINK_DOWN:
        case WAIT_LINK_UP_GET_MODE_DONE:
        case WAIT_LINK_UP_GET_MODE_DONE_RETRY:
        case WAIT_LINK_DOWN_GET_MODE_DONE:
        case READY_LINK_UP:
        case WAIT_DEAD:
            dev_dbg("unexpected timer expiry in state %d\n",state);
            abort();
        break;
    }
}

void
bcm57762::dev::handle_link_up()
{
    switch (state)
    {
        case WAIT_PHY_RESET_DONE:
        case WAIT_PHY_START_AUTONEG_DONE:
        case WAIT_LINK_UP_GET_MODE_DONE_RETRY:
        case READY_LINK_UP:
            // Ignore it.
        break;

        case WAIT_PHY_LINK_NOTIFICATION:
        case READY_LINK_DOWN:
            intf->phy->issue_get_link_mode(&phy_lm_wqe);
            TRANSITION(WAIT_LINK_UP_GET_MODE_DONE);
        break;

        case WAIT_LINK_UP_GET_MODE_DONE:
        case WAIT_LINK_DOWN_GET_MODE_DONE:
            TRANSITION(WAIT_LINK_UP_GET_MODE_DONE_RETRY);
        break;


        case WAIT_RESETTING_FORCED_INTERRUPT:
        case WAIT_RESETTING_NVRAM_LOCK:
        case WAIT_RESETTING_CORE_CLOCK:
        case WAIT_RESETTING_BOOTCODE_DONE:
        case WAIT_RESETTING_COALESCING_STOP:
        case WAIT_RESETTING_DMA_READY:
        case WAIT_RESETTING_GML_READY:
        case WAIT_RESETTING_WDMA_READY:
        case WAIT_RESETTING_RDMA_READY:
        case WAIT_RESETTING_TXMAC_READY:
        case WAIT_RESETTING_RXMAC_READY:
        case WAIT_PHY_PROBE_DONE:
        case WAIT_DEAD:
            dev_dbg("unexpected link up notification in state %d\n",state);
            abort();
        break;
    }
}

void
bcm57762::dev::handle_link_down()
{
    switch (state)
    {
        case WAIT_PHY_RESET_DONE:
        case WAIT_PHY_START_AUTONEG_DONE:
        case READY_LINK_DOWN:
        case WAIT_LINK_DOWN_GET_MODE_DONE:
            // Ignore it.
        break;

        case WAIT_PHY_LINK_NOTIFICATION:
        case READY_LINK_UP:
            dev_dbg("link down\n");
            TRANSITION(READY_LINK_DOWN);
        break;

        case WAIT_LINK_UP_GET_MODE_DONE:
        case WAIT_LINK_UP_GET_MODE_DONE_RETRY:
            TRANSITION(WAIT_LINK_DOWN_GET_MODE_DONE);
        break;

        case WAIT_RESETTING_FORCED_INTERRUPT:
        case WAIT_RESETTING_NVRAM_LOCK:
        case WAIT_RESETTING_CORE_CLOCK:
        case WAIT_RESETTING_BOOTCODE_DONE:
        case WAIT_RESETTING_COALESCING_STOP:
        case WAIT_RESETTING_DMA_READY:
        case WAIT_RESETTING_GML_READY:
        case WAIT_RESETTING_WDMA_READY:
        case WAIT_RESETTING_RDMA_READY:
        case WAIT_RESETTING_TXMAC_READY:
        case WAIT_RESETTING_RXMAC_READY:
        case WAIT_PHY_PROBE_DONE:
        case WAIT_DEAD:
            dev_dbg("unexpected link down notification in state %d\n",state);
            abort();
        break;
    }
}

void
bcm57762::dev::handle_phy_completion()
{
    switch (state)
    {
        case WAIT_PHY_PROBE_DONE:
        case WAIT_PHY_RESET_DONE:
        case WAIT_PHY_START_AUTONEG_DONE:
        case WAIT_LINK_UP_GET_MODE_DONE:
        case WAIT_LINK_UP_GET_MODE_DONE_RETRY:
        case WAIT_LINK_DOWN_GET_MODE_DONE:
            complete_phy_transaction();
        break;

        case WAIT_RESETTING_FORCED_INTERRUPT:
        case WAIT_RESETTING_NVRAM_LOCK:
        case WAIT_RESETTING_CORE_CLOCK:
        case WAIT_RESETTING_BOOTCODE_DONE:
        case WAIT_RESETTING_COALESCING_STOP:
        case WAIT_RESETTING_DMA_READY:
        case WAIT_RESETTING_GML_READY:
        case WAIT_RESETTING_WDMA_READY:
        case WAIT_RESETTING_RDMA_READY:
        case WAIT_RESETTING_TXMAC_READY:
        case WAIT_RESETTING_RXMAC_READY:
        case WAIT_PHY_LINK_NOTIFICATION:
        case READY_LINK_UP:
        case READY_LINK_DOWN:
        case WAIT_DEAD:
            dev_dbg("spurious bcm57762 interrupt in state %d\n",state);
            abort();
        break;
    }
}

void
bcm57762::dev::handle_phy_probe_complete(kernel::work_entry* wqe)
{
    kassert(state == WAIT_PHY_PROBE_DONE);
    if (wqe->args[1])
    {
        dev_dbg("error 0x%08lX probing PHY\n",wqe->args[1]);
        TRANSITION(WAIT_DEAD);
        return;
    }

    intf->phy->issue_reset(&phy_reset_wqe);
    TRANSITION(WAIT_PHY_RESET_DONE);
}

void
bcm57762::dev::handle_phy_reset_complete(kernel::work_entry* wqe)
{
    kassert(state == WAIT_PHY_RESET_DONE);
    if (wqe->args[1])
    {
        dev_dbg("error 0x%08lX resetting PHY\n",wqe->args[1]);
        TRANSITION(WAIT_DEAD);
        return;
    }

    intf->phy->issue_start_autonegotiation(&phy_an_wqe);
    TRANSITION(WAIT_PHY_START_AUTONEG_DONE);
}

void
bcm57762::dev::handle_phy_start_autoneg_complete(kernel::work_entry* wqe)
{
    kassert(state == WAIT_PHY_START_AUTONEG_DONE);
    if (wqe->args[1])
    {
        dev_dbg("error 0x%08lX enabling autopolling\n",wqe->args[1]);
        TRANSITION(WAIT_DEAD);
        return;
    }

    // Enable auto-polling.  This will eventually generate a link-down or a
    // link-up notification.  Note, it seems we need the extra read in there,
    // otherwise with DEBUG_TRANSITIONS=0 we don't get a future interrupt -
    // seems like something needs to be flushed to the chip before reenabling
    // the interrupts.
    reg_set_32((1<<4),0x454);
    reg_read_32(0x454);
    TRANSITION(WAIT_PHY_LINK_NOTIFICATION);
}

void
bcm57762::dev::handle_phy_get_link_mode_complete(kernel::work_entry* wqe)
{
    switch (state)
    {
        case WAIT_LINK_UP_GET_MODE_DONE_RETRY:
        case WAIT_LINK_UP_GET_MODE_DONE:
        case WAIT_LINK_DOWN_GET_MODE_DONE:
            if (wqe->args[1])
            {
                dev_dbg("error 0x%08lX getting link mode\n",wqe->args[1]);
                TRANSITION(WAIT_DEAD);
                return;
            }
        break;

        case WAIT_RESETTING_NVRAM_LOCK:
        case WAIT_RESETTING_CORE_CLOCK:
        case WAIT_RESETTING_BOOTCODE_DONE:
        case WAIT_RESETTING_COALESCING_STOP:
        case WAIT_RESETTING_DMA_READY:
        case WAIT_RESETTING_GML_READY:
        case WAIT_RESETTING_WDMA_READY:
        case WAIT_RESETTING_RDMA_READY:
        case WAIT_RESETTING_TXMAC_READY:
        case WAIT_RESETTING_RXMAC_READY:
        case WAIT_RESETTING_FORCED_INTERRUPT:
        case WAIT_PHY_PROBE_DONE:
        case WAIT_PHY_RESET_DONE:
        case WAIT_PHY_START_AUTONEG_DONE:
        case WAIT_PHY_LINK_NOTIFICATION:
        case READY_LINK_DOWN:
        case READY_LINK_UP:
        case WAIT_DEAD:
            dev_dbg("unexpected get link mode completion in state %d\n",state);
            abort();
        break;
    }

    switch (state)
    {
        case WAIT_LINK_UP_GET_MODE_DONE_RETRY:
            intf->phy->issue_get_link_mode(&phy_lm_wqe);
            TRANSITION(WAIT_LINK_UP_GET_MODE_DONE);
        break;

        case WAIT_LINK_UP_GET_MODE_DONE:
            if (!(wqe->args[3] & PHY_LM_LINK_UP))
            {
                // The MAC thinks it's up, the PHY thinks it's down.  Eventually
                // the MAC will catch up on its next autopoll cycle.
                dev_dbg("link down\n");
                TRANSITION(READY_LINK_DOWN);
                return;
            }

            dev_dbg("link up at %lu Mbit %s duplex\n",
                     wqe->args[2],
                     (wqe->args[3] & PHY_LM_DUPLEX_FULL) ? "full" : "half");
            TRANSITION(READY_LINK_UP);
        break;

        case WAIT_LINK_DOWN_GET_MODE_DONE:
            TRANSITION(READY_LINK_DOWN);
        break;

        default:
            abort();
        break;
    }
}

bcm57762::interface::interface(bcm57762::dev* dev):
    eth::interface(dev->mac,128,128),
    dev(dev)
{
}

bcm57762::interface::~interface()
{
    kernel::panic("So mean.");
}

void
bcm57762::interface::issue_phy_read_16(uint8_t offset, kernel::work_entry* cqe)
{
    dev->issue_phy_read_16(offset,cqe);
}

void
bcm57762::interface::issue_phy_write_16(uint16_t v, uint8_t offset,
    kernel::work_entry* cqe)
{
    return dev->issue_phy_write_16(v,offset,cqe);
}

void
bcm57762::interface::post_tx_frame(eth::tx_op* op)
{
    kernel::panic("Not implemented");
}

void
bcm57762::interface::post_rx_pages(kernel::klist<eth::rx_page>& pages)
{
    kernel::panic("Not implemented");
}
