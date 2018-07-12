#ifndef __KERNEL_NET_BCM57762_BCM57762_H
#define __KERNEL_NET_BCM57762_BCM57762_H

#include "ring.h"
#include "pci_driver.h"
#include "../eth.h"

namespace bcm57762
{
    struct phy;
    struct interface;

    struct registers
    {
        volatile uint8_t    pci_shadow[256];    // 0x0000
        uint8_t             rsrv[256];          // 0x0100
        volatile uint8_t    mailbox[512];       // 0x0200
        volatile uint8_t    registers[31744];   // 0x0400
        volatile uint8_t    window[32768];      // 0x8000
    } __PACKED__;
    KASSERT(sizeof(registers) == 65536);
    KASSERT(offsetof(registers,pci_shadow) == 0x0000);
    KASSERT(offsetof(registers,rsrv)       == 0x0100);
    KASSERT(offsetof(registers,mailbox)    == 0x0200);
    KASSERT(offsetof(registers,registers)  == 0x0400);
    KASSERT(offsetof(registers,window)     == 0x8000);

    struct msix_sv_rss_status_block
    {
        volatile be_uint32_t    status_word;
        volatile uint8_t        rsrv1[3];
        volatile uint8_t        tag;
        volatile be_uint16_t    rx_standard_producer_ring_cons_index;
        volatile be_uint16_t    rx_return_ring_1_prod_index;
        volatile be_uint16_t    rx_return_ring_2_prod_index;
        volatile be_uint16_t    rx_return_ring_3_prod_index;
        volatile be_uint16_t    tx_bd_consumer_index;
        volatile be_uint16_t    rx_return_ring_0_prod_index;
        volatile be_uint16_t    rsrv2;
        volatile be_uint16_t    rx_jumbo_producer_ring_cons_index;
        volatile uint8_t        rsrv3[8];
    } __PACKED__;
    KASSERT(sizeof(msix_sv_rss_status_block) == 32);

    struct mem_block
    {
        msix_sv_rss_status_block    status_block;
        uint8_t                     rsrv[4064];
        uint8_t                     rsrv2[4096];
        send_bd                     send_bds[512];
        receive_bd                  recv_bds[512];
        receive_bd                  return_bds[512];
        uint8_t                     rsrv3[16384];
    } __PACKED__;
    KASSERT(sizeof(mem_block) == 65536);
    KASSERT(kernel::is_pow2(sizeof(mem_block)));
    KASSERT(kernel::is_pow2(FIELD_NELEMS(mem_block,send_bds)));
    KASSERT(kernel::is_pow2(FIELD_NELEMS(mem_block,recv_bds)));
    KASSERT(FIELD_NELEMS(mem_block,recv_bds) ==
            FIELD_NELEMS(mem_block,return_bds));

    struct dev : public kernel::pci::dev
    {
        registers*              regs;
        mem_block*              mem;
        bcm57762::interface*    intf;
        uint16_t                tx_bd_producer_index;
        eth::addr               mac;

        void    mem_read(void* dst, uint32_t addr, size_t len);
        void    mem_write(uint32_t addr, const void* src, size_t len);

        uint32_t    reg_read_32(uint32_t offset);
        void        reg_write_32(uint32_t val, uint32_t offset);
        inline void reg_set_clear_32(uint32_t s, uint32_t c, uint32_t offset)
        {
            kernel::kassert(!(s & c));
            uint32_t v = reg_read_32(offset);
            reg_write_32((v | s) & ~c,offset);
        }
        inline void reg_set_32(uint32_t s, uint16_t offset)
            {reg_set_clear_32(s,0,offset);}
        inline void reg_clear_32(uint32_t c, uint16_t offset)
            {reg_set_clear_32(0,c,offset);}
        inline void reg_insert_32(uint32_t val, uint8_t low, uint8_t high,
                                  uint16_t offset)
        {
            kernel::kassert(low <= high);
            kernel::kassert(high < 32);
            val <<= low;
            uint32_t mask = ((1 << (high+1)) - 1) ^ ((1 << low) - 1);
            kernel::kassert(!(val & ~mask));
            reg_write_32((reg_read_32(offset) & ~mask) | val,offset);
        }

        uint16_t    phy_read_16(uint8_t offset);
        void        phy_write_16(uint16_t v, uint8_t offset);

        // Interrupt handling.
        void    mask_inta_interrupts();
        void    handle_vec0_dsr(kernel::work_entry*);

        // Actions.
        void    issue_reset();

        dev(const kernel::pci::dev* pd, const bcm57762::driver* owner);
        virtual ~dev();
    };

    struct interface : public eth::interface
    {
        bcm57762::dev*  dev;

        virtual uint16_t    phy_read_16(uint8_t offset);
        virtual void        phy_write_16(uint16_t v, uint8_t offset);

        virtual void    post_tx_frame(eth::tx_op* op);
        virtual void    post_rx_pages(kernel::klist<eth::rx_page>& pages);

        interface(bcm57762::dev* vdev);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_BCM57762_BCM57762_H */
