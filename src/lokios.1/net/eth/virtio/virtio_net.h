#ifndef __KERNEL_NET_VIRTIO_NET_H
#define __KERNEL_NET_VIRTIO_NET_H

#include "virtio_queue.h"
#include "net/eth/interface.h"
#include "pci/pci.h"
#include "mm/mm.h"

namespace virtio_net
{
    struct interface;

#define VIRTIO_PCI_CAP_COMMON_CFG   1
#define VIRTIO_PCI_CAP_NOTIFY_CFG   2
#define VIRTIO_PCI_CAP_ISR_CFG      3
#define VIRTIO_PCI_CAP_DEVICE_CFG   4
#define VIRTIO_PCI_CAP_PCI_CFG      5
    struct pci_cap
    {
        uint8_t     vndr;
        uint8_t     next;
        uint8_t     len;
        uint8_t     type;
        uint8_t     bar;
        uint8_t     padding[3];
        uint32_t    offset;
        uint32_t    length;
    };
    KASSERT(sizeof(pci_cap) == 16);

    struct common_cfg
    {
        volatile uint32_t    device_feature_select;
        volatile uint32_t    device_feature;
        volatile uint32_t    driver_feature_select;
        volatile uint32_t    driver_feature;
        volatile uint16_t    msix_config;
        volatile uint16_t    num_queues;
        volatile uint8_t     device_status;
        volatile uint8_t     config_generation;

        volatile uint16_t    queue_select;
        volatile uint16_t    queue_size;
        volatile uint16_t    queue_msix_vector;
        volatile uint16_t    queue_enable;
        volatile uint16_t    queue_notify_off;
        volatile uint64_t    queue_desc;
        volatile uint64_t    queue_avail;
        volatile uint64_t    queue_used;
    };
    KASSERT(sizeof(common_cfg) == 56);

// Virtio-net feature bits.
#define VIRTIO_NET_F_CSUM                   (1UL<<0)
#define VIRTIO_NET_F_GUEST_CSUM             (1UL<<1)
#define VIRTIO_NET_F_CTRL_GUEST_OFFLOADS    (1UL<<2)
#define VIRTIO_NET_F_MAC                    (1UL<<5)
#define VIRTIO_NET_F_GUEST_TSO4             (1UL<<7)
#define VIRTIO_NET_F_GUEST_TSO6             (1UL<<8)
#define VIRTIO_NET_F_GUEST_ECN              (1UL<<9)
#define VIRTIO_NET_F_GUEST_UFO              (1UL<<10)
#define VIRTIO_NET_F_HOST_TSO4              (1UL<<11)
#define VIRTIO_NET_F_HOST_TSO6              (1UL<<12)
#define VIRTIO_NET_F_HOST_ECN               (1UL<<13)
#define VIRTIO_NET_F_HOST_UFO               (1UL<<14)
#define VIRTIO_NET_F_MRG_RXBUF              (1UL<<15)
#define VIRTIO_NET_F_STATUS                 (1UL<<16)
#define VIRTIO_NET_F_CTRL_VQ                (1UL<<17)
#define VIRTIO_NET_F_CTRL_RX                (1UL<<18)
#define VIRTIO_NET_F_CTRL_VLAN              (1UL<<19)
#define VIRTIO_NET_F_GUEST_ANNOUNCE         (1UL<<21)
#define VIRTIO_NET_F_MQ                     (1UL<<22)
#define VIRTIO_NET_F_CTRL_MAC_ADDR          (1UL<<23)

// Generic feature bits.
#define VIRTIO_F_RING_INDIRECT_DESC         (1UL<<28)
#define VIRTIO_F_RING_EVENT_IDX             (1UL<<29)
#define VIRTIO_F_VERSION_1                  (1UL<<32)

    struct net_config
    {
        eth::addr   mac;
        uint16_t    status;
        uint16_t    max_virtqueue_pairs;
    };

    struct set_mac_addr_cmd
    {
        uint8_t             cls;
        uint8_t             command;
        eth::addr           mac;
        volatile uint8_t    ack;
    };

#define VIRTIO_NET_HDR_F_NEEDS_CSUM    1
#define VIRTIO_NET_HDR_GSO_NONE        0
#define VIRTIO_NET_HDR_GSO_TCPV4       1
#define VIRTIO_NET_HDR_GSO_UDP         3
#define VIRTIO_NET_HDR_GSO_TCPV6       4
#define VIRTIO_NET_HDR_GSO_ECN      0x80
    struct net_hdr
    {
        uint8_t     flags;
        uint8_t     gso_type;
        uint16_t    hdr_len;
        uint16_t    gso_size;
        uint16_t    csum_start;
        uint16_t    csum_offset;
        uint16_t    num_buffers;
    };

    struct driver : public kernel::pci::driver
    {
        virtual uint64_t score(kernel::pci::dev* pd) const;
        virtual kernel::pci::dev* claim(kernel::pci::dev* pd) const;
        virtual void release(kernel::pci::dev* pd) const;
        driver();
    };

    struct dev : public kernel::pci::dev
    {
        enum
        {
            VN_STATE_IDLE,
            VN_STATE_WAIT_RESET,
            VN_STATE_WAIT_SET_MAC_COMP,
            VN_STATE_DONE,
        } state;

        virtio_net::interface*      intf;

        struct common_cfg*          common_cfg;
        net_config*                 device_cfg;
        void*                       notify_cfg;
        uint32_t                    notify_offset_multiplier;
        eth::addr                   mac;
        kernel::timer_entry         timer_wqe;
        
        vqueue                      rq;
        vqueue                      tq;
        vqueue                      cq;

        net::tx_op*                 tx_table[256];
        kernel::klist<net::tx_op>   tx_send_queue;

        uint64_t    get_device_features();
        void        set_driver_features(uint64_t f);
        uint16_t*   get_queue_notify_addr();
        void        arm_timer(uint64_t dt10ms);

        void        issue_reset();
        void        issue_set_mac_address();

        void        post_tx_frame(net::tx_op* op);
        void        process_send_queue();
        void        post_rx_pages(kernel::klist<net::rx_page>& pages);
            
        void        handle_timer(kernel::timer_entry*);
        void        handle_rq_dsr(kernel::work_entry*);
        void        handle_tq_dsr(kernel::work_entry*);
        void        handle_cq_dsr(kernel::work_entry*);
        void        handle_cq_completion(uint16_t index);
        void        handle_config_change_dsr(kernel::work_entry*);

        dev(const kernel::pci::dev* pd, const virtio_net::driver* owner);
    };
}

#endif /* __KERNEL_NET_VIRTIO_NET_H */
