/*
 * Implements an ARP querier.  Useful references:
 *
 * https://en.wikipedia.org/wiki/Address_Resolution_Protocol
 * https://tools.ietf.org/pdf/rfc826.pdf
 */
#ifndef __KERNEL_NET_ARP_H
#define __KERNEL_NET_ARP_H

#include "net/eth/interface.h"
#include "kernel/schedule.h"
#include "kernel/cpu.h"
#include "mm/slab.h"
#include "mm/mm.h"

#include <kernel/console.h>

namespace arp
{
    struct short_payload
    {
        be_uint16_t htype;
        be_uint16_t ptype;
        uint8_t     hlen;
        uint8_t     plen;
        be_uint16_t oper;
    } __PACKED__;

    template<typename hw_traits, typename proto_traits>
    struct payload
    {
        typedef typename hw_traits::addr_type       hw_addr;
        typedef typename proto_traits::addr_type    proto_addr;

        be_uint16_t htype;
        be_uint16_t ptype;
        uint8_t     hlen;
        uint8_t     plen;
        be_uint16_t oper;
        hw_addr     sha;
        proto_addr  spa;
        hw_addr     tha;
        proto_addr  tpa;
    } __PACKED__;

    template<typename hw_traits, typename proto_traits>
    struct frame
    {
        typedef typename hw_traits::header_type header_type;
        
        header_type                     hdr;
        payload<hw_traits,proto_traits> msg;
    } __PACKED__;

    template<typename hw_traits, typename proto_traits>
    struct entry
    {
        typedef typename hw_traits::addr_type       _hw_addr;
        typedef typename proto_traits::addr_type    _proto_addr;

        _hw_addr            hw_addr;
        _proto_addr         proto_addr;
        kernel::timer_entry expiry_wqe;
    };

    template<typename hw_traits, typename proto_traits>
    struct service;

    template<typename hw_traits, typename proto_traits>
    struct lookup_op
    {
        typedef typename hw_traits::addr_type       _hw_addr;
        typedef typename hw_traits::tx_op_type      _tx_op;
        typedef typename proto_traits::addr_type    _proto_addr;

        kernel::kdlink                          link;

        enum op_state_type
        {
            WAIT_POST,
            WAIT_RX_RESP_TX_COMP,
            WAIT_RX_RESP,
            WAIT_TX_COMP,
        } state;

        arp::service<hw_traits,proto_traits>*   service;
        _tx_op                                  tx_op;
        kernel::work_entry*                     cqe;
        kernel::timer_entry                     timeout_cqe;
        uint64_t                                timeout_ms;
        _hw_addr*                               tha;
        arp::frame<hw_traits,proto_traits>      frame;

        static void send_cb(_tx_op* top)
        {
            auto* op = container_of(top,lookup_op,tx_op);
            op->handle_lookup_send_comp();
        }

        void post()
        {
            kernel::kassert(state == WAIT_POST);
            state = WAIT_RX_RESP_TX_COMP;
            service->intf->post_tx_frame(&tx_op);
        }

        void handle_lookup_send_comp()
        {
            kernel::console::printf("arp: lookup tx send completion\n");
            switch (state)
            {
                case WAIT_POST:
                    kernel::panic("send completion without posting!");
                break;

                case WAIT_RX_RESP_TX_COMP:
                    state = WAIT_RX_RESP;
                    kernel::cpu::schedule_timer_ms(&timeout_cqe,timeout_ms);
                break;

                case WAIT_RX_RESP:
                    kernel::panic("duplicate send completion");
                break;

                case WAIT_TX_COMP:
                    service->complete_lookup(this,0);
                break;
            }
        }

        void handle_rx_reply_tha(_hw_addr _tha)
        {
            switch (state)
            {
                case WAIT_RX_RESP_TX_COMP:
                    *tha  = _tha;
                    state = WAIT_TX_COMP;
                break;

                case WAIT_RX_RESP:
                    kernel::cpu::cancel_timer(&timeout_cqe);
                    *tha = _tha;
                    service->complete_lookup(this,0);
                break;

                case WAIT_POST:
                case WAIT_TX_COMP:
                break;
            }
        }

        void handle_lookup_timeout(kernel::timer_entry* timeout_cqe)
        {
            switch (state)
            {
                case WAIT_POST:
                case WAIT_RX_RESP_TX_COMP:
                case WAIT_TX_COMP:
                    kernel::panic("timeout without arming!");
                break;

                case WAIT_RX_RESP:
                    service->complete_lookup(this,1);
                break;
            }
        }

        lookup_op(typeof(service) service, _proto_addr tpa, _hw_addr* tha,
                  kernel::work_entry* cqe, size_t timeout_ms):
            state(WAIT_POST),
            service(service),
            cqe(cqe),
            timeout_ms(timeout_ms),
            tha(tha)
        {
            timeout_cqe.fn       = timer_delegate(handle_lookup_timeout);
            timeout_cqe.args[0]  = (uintptr_t)this;
            frame.hdr.dst_mac    = eth::broadcast_addr;
            frame.hdr.src_mac    = service->intf->hw_mac;
            frame.hdr.ether_type = 0x0806;
            frame.msg.htype      = hw_traits::arp_hw_type;
            frame.msg.ptype      = proto_traits::ether_type;
            frame.msg.hlen       = sizeof(frame.msg.sha);
            frame.msg.plen       = sizeof(frame.msg.spa);
            frame.msg.oper       = 1;
            frame.msg.sha        = service->intf->hw_mac;
            frame.msg.spa        = service->intf->ip_addr;
            frame.msg.tha        = eth::addr{0,0,0,0,0,0};
            frame.msg.tpa        = tpa;
            tx_op.cb             = send_cb;
            tx_op.nalps          = 1;
            tx_op.alps[0].paddr  = kernel::virt_to_phys(&frame);
            tx_op.alps[0].len    = sizeof(frame);
        }
    };

    template<typename hw_traits, typename proto_traits>
    struct service
    {
        typedef frame<hw_traits,proto_traits>       arp_frame;
        typedef entry<hw_traits,proto_traits>       arp_entry;
        typedef lookup_op<hw_traits,proto_traits>   arp_lookup_op;
        typedef typename arp_lookup_op::_tx_op      arp_tx_op;
        typedef typename proto_traits::addr_type    arp_proto_addr;
        typedef typename hw_traits::addr_type       arp_hw_addr;
        typedef typename hw_traits::interface_type  arp_interface;

        arp_interface* const            intf;
        kernel::slab                    arp_lookup_ops_slab;
        kernel::kdlist<arp_lookup_op>   arp_lookup_ops;
        kernel::slab                    arp_entries_slab;
        kernel::kdlist<arp_entry>       arp_entries;

        arp_lookup_op* find_lookup(arp_proto_addr tpa)
        {
            for (auto& op : klist_elems(arp_lookup_ops,link))
            {
                if (op.frame.msg.tpa == tpa)
                    return &op;
            }
            return NULL;
        }

        void complete_lookup(arp_lookup_op* op, uint64_t result)
        {
            op->cqe->args[1] = result;
            kernel::cpu::schedule_local_work(op->cqe);
            op->link.unlink();
            arp_lookup_ops_slab.free(op);
        }

        void enqueue_lookup(arp_proto_addr tpa, arp_hw_addr* tha,
                            kernel::work_entry* cqe, size_t timeout_ms)
        {
            arp_lookup_op* op = arp_lookup_ops_slab.alloc<arp_lookup_op>(
                    this,tpa,tha,cqe,timeout_ms);
            arp_lookup_ops.push_back(&op->link);
            op->post();
        }

        void handle_rx_frame(eth::rx_page* p)
        {
            kernel::console::printf("arp: handle rx frame\n");
            auto* f = (arp_frame*)(p->payload + p->eth_offset);
            if (f->msg.oper == 2)
                handle_rx_reply_frame(p);
        }

        void handle_rx_reply_frame(eth::rx_page* p)
        {
            kernel::console::printf("arp: handle rx reply frame\n");
            auto* f           = (arp_frame*)(p->payload + p->eth_offset);
            arp_lookup_op* op = find_lookup(f->msg.spa);
            if (op)
                op->handle_rx_reply_tha(f->msg.tha);
            else
            {
                kernel::console::printf("arp: couldn't find lookup op for "
                                        "%u.%u.%u.%u\n",
                                        f->msg.spa[0],f->msg.spa[1],
                                        f->msg.spa[2],f->msg.spa[3]);
            }
        }

        service(arp_interface* intf):
            intf(intf),
            arp_lookup_ops_slab(sizeof(arp_lookup_op)),
            arp_entries_slab(sizeof(arp_entry))
        {
        }
    };
}

#endif /* __KERNEL_NET_ARP_H */
