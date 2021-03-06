#ifndef __KERNEL_NET_BCM57762_BCM57762_H
#define __KERNEL_NET_BCM57762_BCM57762_H

#include "ring.h"
#include "pci_driver.h"
#include "net/eth/interface.h"

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
        net::rx_page*               rx_page_table[512];
        send_bd                     send_bds[512];
        receive_bd                  recv_bds[512];
        receive_bd                  return_bds[512];
        net::tx_op*                 tx_op_table[512];
        uint8_t                     rsrv3[12288];
    } __PACKED__;
    KASSERT(sizeof(mem_block) == 65536);
    KASSERT(kernel::is_pow2(sizeof(mem_block)));
    KASSERT(kernel::is_pow2(nelmof_field(mem_block,send_bds)));
    KASSERT(kernel::is_pow2(nelmof_field(mem_block,recv_bds)));
    KASSERT(nelmof_field(mem_block,recv_bds) ==
            nelmof_field(mem_block,return_bds));

    struct dev : public kernel::pci::dev
    {
        enum dev_state
        {
            WAIT_RESETTING_NVRAM_LOCK,
            WAIT_RESETTING_CORE_CLOCK,
            WAIT_RESETTING_BOOTCODE_DONE,
            WAIT_RESETTING_COALESCING_STOP,
            WAIT_RESETTING_DMA_READY,
            WAIT_RESETTING_GML_READY,
            WAIT_RESETTING_WDMA_READY,
            WAIT_RESETTING_RDMA_READY,
            WAIT_RESETTING_TXMAC_READY,
            WAIT_RESETTING_RXMAC_READY,
            WAIT_RESETTING_FORCED_INTERRUPT,

            WAIT_PHY_PROBE_DONE,
            WAIT_PHY_RESET_DONE,
            WAIT_PHY_START_AUTONEG_DONE,
            WAIT_PHY_LINK_NOTIFICATION,

            READY_LINK_DOWN,
            WAIT_LINK_UP_GET_MODE_DONE,
            WAIT_LINK_UP_GET_MODE_DONE_RETRY,
            WAIT_LINK_DOWN_GET_MODE_DONE,
            READY_LINK_UP,
            
            WAIT_DEAD,
        } state;
        size_t                  timer_retries;
        kernel::tqe             timer_wqe;
        kernel::wqe             phy_probe_wqe;
        kernel::wqe             phy_reset_wqe;
        kernel::wqe             phy_an_wqe;
        kernel::wqe             phy_lm_wqe;

        registers*              regs;
        mem_block*              mem;
        bcm57762::interface*    intf;
        uint16_t                tx_bd_producer_index;
        uint16_t                tx_bd_consumer_index;
        uint16_t                rx_bd_producer_index;
        uint16_t                rx_bd_consumer_index;
        eth::addr               mac;
        kernel::wqe*            phy_cqe;
        bool                    phy_autopoll;

        kernel::klist<net::tx_op>   tx_send_queue;

        // Synchronous register accessors.
        void        mem_read(void* dst, uint32_t addr, size_t len);
        void        mem_write(uint32_t addr, const void* src, size_t len);
        uint32_t    reg_read_32(uint32_t offset);
        void        reg_write_32(uint32_t val, uint32_t offset);

        // Async PHY register accessors.  These go via some sort of serial
        // interface and we get interrupt completions when the accesses
        // complete.  An error code will be populated in args[1] and the
        // transferred data will be populated in cqe->args[2].
        void    issue_phy_read_16(uint8_t offset, kernel::wqe* cqe);
        void    issue_phy_write_16(uint16_t v, uint8_t offset,
                                   kernel::wqe* cqe);
        void    begin_phy_transaction(uint32_t v);
        void    complete_phy_transaction();

        // Register field helpers.
        inline void reg_set_clear_32(uint32_t s, uint32_t c, uint32_t offset)
        {
            kernel::kassert(!(s & c));
            uint32_t v = reg_read_32(offset);
            reg_write_32((v | s) & ~c,offset);
        }
        inline void reg_set_32(uint32_t s, uint16_t offset)
        {
            reg_set_clear_32(s,0,offset);
        }
        inline void reg_clear_32(uint32_t c, uint16_t offset)
        {
            reg_set_clear_32(0,c,offset);
        }
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

        // Actions.
        void    issue_reset();

        // Handlers.
        void    handle_vec0_dsr(kernel::wqe*);
        void    handle_timer(kernel::tqe*);
        void    handle_link_up();
        void    handle_link_down();
        void    handle_phy_completion();
        void    handle_phy_probe_complete(kernel::wqe*);
        void    handle_phy_reset_complete(kernel::wqe*);
        void    handle_phy_start_autoneg_complete(kernel::wqe*);
        void    handle_phy_get_link_mode_complete(kernel::wqe*);

        // Interface API.
        void    post_tx_frame(net::tx_op* op);
        void    process_send_queue();
        void    post_rx_pages(kernel::klist<net::rx_page>& pages);

        dev(const kernel::pci::dev* pd, const bcm57762::driver* owner);
        virtual ~dev();
    };

    struct interface : public eth::interface
    {
        bcm57762::dev*  dev;

        virtual void    issue_phy_read_16(uint8_t offset, kernel::wqe* cqe);
        virtual void    issue_phy_write_16(uint16_t v, uint8_t offset,
                                           kernel::wqe* cqe);

        virtual void    post_tx_frame(net::tx_op* op);
        virtual void    post_rx_pages(kernel::klist<net::rx_page>& pages);

        virtual void    add_driver_wapi_info(http::response* rsp);

        interface(bcm57762::dev* vdev);
        virtual ~interface();
    };
}

#endif /* __KERNEL_NET_BCM57762_BCM57762_H */
