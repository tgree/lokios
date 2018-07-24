/*
 * Implements an ARP querier.  Useful references:
 *
 * https://en.wikipedia.org/wiki/Address_Resolution_Protocol
 * https://tools.ietf.org/pdf/rfc826.pdf
 */
#ifndef __KERNEL_NET_ARP_H
#define __KERNEL_NET_ARP_H

#include "net/net.h"
#include "kernel/schedule.h"
#include "kernel/cpu.h"
#include "mm/slab.h"
#include "mm/mm.h"

#include <kernel/console.h>

namespace arp
{
    struct header
    {
        be_uint16_t htype;
        be_uint16_t ptype;
        uint8_t     hlen;
        uint8_t     plen;
        be_uint16_t oper;
    } __PACKED__;

    template<typename hw_traits, typename proto_traits>
    struct service
    {
        typedef typename proto_traits::addr_type    proto_addr;
        typedef typename hw_traits::addr_type       hw_addr;
        typedef typename hw_traits::interface_type  interface;
        typedef typename hw_traits::header_type     ll_header_type;

        struct arp_frame
        {
            ll_header_type  llhdr;
            arp::header     hdr;
            hw_addr         sha;
            proto_addr      spa;
            hw_addr         tha;
            proto_addr      tpa;
        } __PACKED__;

        struct entry
        {
            kernel::kdlink      link;
            hw_addr             ha;
            proto_addr          pa;
            kernel::timer_entry expiry_wqe;
        };

        struct lookup_op
        {
            kernel::kdlink link;

            enum op_state_type
            {
                WAIT_POST,
                WAIT_RX_RESP_TX_COMP,
                WAIT_RX_RESP,
                WAIT_TX_COMP,
            } state;

            service*            serv;
            net::tx_op          op;
            kernel::work_entry* cqe;
            kernel::timer_entry timeout_cqe;
            uint64_t            timeout_ms;
            hw_addr*            tha;
            arp_frame           frame;

            static void send_cb(net::tx_op* top)
            {
                container_of(top,lookup_op,op)->handle_lookup_send_comp();
            }

            void post()
            {
                kernel::kassert(state == WAIT_POST);
                state = WAIT_RX_RESP_TX_COMP;
                serv->intf->post_tx_frame(&op);
            }

            void handle_lookup_send_comp()
            {
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
                        serv->complete_lookup(this,0);
                    break;
                }
            }

            void handle_rx_reply_tha(hw_addr _tha)
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
                        serv->complete_lookup(this,0);
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
                        serv->complete_lookup(this,1);
                    break;
                }
            }

            lookup_op(service* serv, proto_addr tpa, hw_addr* tha,
                      kernel::work_entry* cqe, size_t timeout_ms):
                state(WAIT_POST),
                serv(serv),
                cqe(cqe),
                timeout_ms(timeout_ms),
                tha(tha)
            {
                timeout_cqe.fn         = timer_delegate(handle_lookup_timeout);
                timeout_cqe.args[0]    = (uintptr_t)this;
                frame.llhdr.dst_mac    = hw_traits::broadcast_addr;
                frame.llhdr.src_mac    = serv->intf->hw_mac;
                frame.llhdr.ether_type = 0x0806;
                frame.hdr.htype        = hw_traits::arp_hw_type;
                frame.hdr.ptype        = proto_traits::ether_type;
                frame.hdr.hlen         = sizeof(frame.sha);
                frame.hdr.plen         = sizeof(frame.spa);
                frame.hdr.oper         = 1;
                frame.sha              = serv->intf->hw_mac;
                frame.spa              = serv->intf->ip_addr;
                frame.tha              = hw_traits::zero_addr;
                frame.tpa              = tpa;
                op.cb                  = send_cb;
                op.flags               = 0;
                op.nalps               = 1;
                op.alps[0].paddr       = kernel::virt_to_phys(&frame);
                op.alps[0].len         = sizeof(frame);
            }
        };

        struct reply_op
        {
            service*    serv;
            net::tx_op  op;
            arp_frame   frame;

            static void send_cb(net::tx_op* top)
            {
                container_of(top,reply_op,op)->handle_reply_send_comp();
            }

            void post()
            {
                serv->intf->post_tx_frame(&op);
            }

            void handle_reply_send_comp()
            {
                serv->handle_reply_send_comp(this);
            }

            reply_op(service* serv, typeof(frame)* req):
                serv(serv)
            {
                memcpy(&frame,req,sizeof(frame));
                frame.llhdr.dst_mac = req->llhdr.src_mac;
                frame.llhdr.src_mac = serv->intf->hw_mac;
                frame.hdr.oper      = 2;
                frame.sha           = serv->intf->hw_mac;
                frame.spa           = serv->intf->ip_addr;
                frame.tha           = req->sha;
                frame.tpa           = req->spa;
                op.cb               = send_cb;
                op.nalps            = 1;
                op.alps[0].paddr    = kernel::virt_to_phys(&frame);
                op.alps[0].len      = sizeof(frame);
            }
        };

        interface* const                intf;
        kernel::slab                    arp_lookup_ops_slab;
        kernel::kdlist<lookup_op>       arp_lookup_ops;
        kernel::slab                    arp_reply_ops_slab;
        kernel::slab                    arp_entries_slab;
        kernel::kdlist<entry>           arp_entries;

        entry* find_entry(proto_addr tpa)
        {
            for (auto& e : klist_elems(arp_entries,link))
            {
                if (e.pa == tpa)
                    return &e;
            }
            return NULL;
        }

        void add_entry(proto_addr pa, hw_addr ha)
        {
            auto* e  = arp_entries_slab.alloc<entry>();
            e->ha    = ha;
            e->pa    = pa;
            arp_entries.push_back(&e->link);
        }

        lookup_op* find_lookup(proto_addr tpa)
        {
            for (auto& op : klist_elems(arp_lookup_ops,link))
            {
                if (op.frame.tpa == tpa)
                    return &op;
            }
            return NULL;
        }

        void complete_lookup(lookup_op* op, uint64_t result)
        {
            op->cqe->args[1] = result;
            kernel::cpu::schedule_local_work(op->cqe);
            op->link.unlink();
            arp_lookup_ops_slab.free(op);
        }

        void enqueue_lookup(proto_addr tpa, hw_addr* tha,
                            kernel::work_entry* cqe, size_t timeout_ms)
        {
            lookup_op* op = arp_lookup_ops_slab.alloc<lookup_op>(this,tpa,tha,
                                cqe,timeout_ms);
            arp_lookup_ops.push_back(&op->link);
            op->post();
        }

        void handle_rx_frame(net::rx_page* p)
        {
            auto* f = (arp_frame*)(p->payload + p->pay_offset);
            auto* e = find_entry(f->spa);
            if (e)
                e->ha = f->sha;
            if (f->tpa == intf->ip_addr)
            {
                if (!e)
                    add_entry(f->spa,f->sha);
                if (f->hdr.oper == 2)
                    handle_rx_reply_frame(p);
                else
                    handle_rx_request_frame(p);
            }
        }

        void handle_rx_reply_frame(net::rx_page* p)
        {
            kernel::console::printf("arp: handle rx reply frame\n");
            auto* f       = (arp_frame*)(p->payload + p->pay_offset);
            lookup_op* op = find_lookup(f->spa);
            if (op)
                op->handle_rx_reply_tha(f->tha);
            else
            {
                kernel::console::printf("arp: couldn't find lookup op for "
                                        "%u.%u.%u.%u\n",
                                        f->spa[0],f->spa[1],
                                        f->spa[2],f->spa[3]);
            }
        }

        void handle_rx_request_frame(net::rx_page* p)
        {
            kernel::console::printf("arp: handle rx request frame\n");
            auto* f      = (arp_frame*)(p->payload + p->pay_offset);
            reply_op* op = arp_reply_ops_slab.alloc<reply_op>(this,f);
            op->post();
        }

        void handle_reply_send_comp(reply_op* op)
        {
            arp_reply_ops_slab.free(op);
        }

        service(interface* intf):
            intf(intf),
            arp_lookup_ops_slab(sizeof(lookup_op)),
            arp_reply_ops_slab(sizeof(reply_op)),
            arp_entries_slab(sizeof(entry))
        {
        }
    };
}

#endif /* __KERNEL_NET_ARP_H */
