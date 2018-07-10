#include "syslogger.h"
#include "arp.h"
#include "dhcpc.h"
#include <k++/string_stream.h>

using kernel::console::printf;

static void
handle_send_comp_bounce(eth::tx_op* op)
{
    auto* c = container_of(op,net::syslogger,send_op);
    c->handle_tx_send_comp();
}

net::syslogger::syslogger(eth::interface* intf):
    intf(intf)
{
    arp_cqe.fn      = work_delegate(handle_arp_completion);
    arp_cqe.args[0] = (uintptr_t)this;

    send_op.cb            = handle_send_comp_bounce;
    send_op.nalps         = 1;
    send_op.alps[0].paddr = kernel::virt_to_phys(&pkt);
    send_op.alps[0].len   = sizeof(pkt);
}

void
net::syslogger::post_message()
{
    kernel::string_stream ss(pkt.payload,sizeof(pkt.payload));
    ss.printf("Hello\r\n");

    pkt.llhdr.src_mac          = intf->hw_mac;
    pkt.llhdr.dst_mac          = gw_mac;
    pkt.llhdr.ether_type       = ipv4::net_traits::ether_type;
    pkt.iphdr.version_ihl      = 0x45;
    pkt.iphdr.dscp_ecn         = 0;
    pkt.iphdr.total_len        = sizeof(pkt) - sizeof(pkt.llhdr);
    pkt.iphdr.identification   = 0;
    pkt.iphdr.flags_fragoffset = 0x4000;
    pkt.iphdr.ttl              = 1;
    pkt.iphdr.proto            = udp::net_traits::ip_proto;
    pkt.iphdr.src_ip           = intf->ip_addr;
    pkt.iphdr.dst_ip           = ipv4::addr{127,0,0,1};
    pkt.iphdr.header_checksum  = 0;
    pkt.iphdr.header_checksum  = ipv4::csum(&pkt.iphdr);
    pkt.uhdr.src_port          = 12345;
    pkt.uhdr.dst_port          = 12345;
    pkt.uhdr.len               = sizeof(pkt.uhdr) + ss.strlen();
    pkt.uhdr.checksum          = 0;
    intf->post_tx_frame(&send_op);
}

void
net::syslogger::start()
{
    printf("syslogger: starting\n");
    intf->arpc_ipv4->enqueue_lookup(intf->dhcpc->gw_addr,&gw_mac,&arp_cqe,1000);
}

void
net::syslogger::handle_arp_completion(kernel::work_entry* wqe)
{
    if (wqe->args[1])
    {
        // ARP failure.  Try again.
        start();
        return;
    }

    post_message();
}

void
net::syslogger::handle_tx_send_comp()
{
    post_message();
}
