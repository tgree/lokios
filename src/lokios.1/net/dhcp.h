#ifndef __KERNEL_NET_DHCP_H
#define __KERNEL_NET_DHCP_H

#include "dhcp_option.h"
#include "eth.h"
#include "udp.h"

namespace dhcp
{
#define DHCP_OPTIONS_MAGIC  0x63825363
    struct message
    {
        uint8_t     op;
        uint8_t     htype;
        uint8_t     hlen;
        uint8_t     hops;

        uint32_t    xid;

        be_uint16_t secs;
        be_uint16_t flags;

        ipv4::addr  ciaddr;
        ipv4::addr  yiaddr;
        ipv4::addr  siaddr;
        ipv4::addr  giaddr;

        uint8_t     chaddr[16];

        uint8_t     sname[64];

        uint8_t     file[128];

        be_uint32_t magic;
        uint8_t     options[308];

        void    format_discover(uint32_t xid, const eth::addr& src_mac);
        void    format_request(uint32_t xid, const eth::addr& src_mac,
                               const ipv4::addr& req_addr,
                               const ipv4::addr& server_id);

        const option* find_option(uint8_t tag) const;
        template<typename O>
        auto get_option() const
        {
            auto* o = find_option(O::tag);
            if (!o)
                throw missing_option_exception(O::tag,typeid(O).name());
            return O().parse_option(o);
        }
        template<typename O>
        auto get_option(decltype(O().parse_option((option*)0)) d) const
        {
            auto* o = find_option(O::tag);
            return (o ? O().parse_option(o) : d);
        }

        void append_option(uint8_t tag, uint8_t len, const void* data);
        inline void append_message_type(message_type type)
        {
            uint8_t opt[1] = {type};
            append_option(53,sizeof(opt),opt);
        }

        inline void append_requested_ip(const ipv4::addr& req_addr)
        {
            append_option(50,sizeof(req_addr.v),req_addr.v);
        }

        inline void append_server_id(const ipv4::addr& server_id)
        {
            append_option(54,sizeof(server_id.v),server_id.v);
        }
    } __PACKED__;
    KASSERT(sizeof(message) == 548);

    template<typename LLHeader>
    struct full_message
    {
        typedef typename LLHeader::addr_type lladdr;

        LLHeader        llhdr;
        ipv4::header    iphdr;
        udp::header     uhdr;
        dhcp::message   msg;
    } __PACKED__;
    KASSERT(sizeof(dhcp::full_message<eth::header>) ==
            548 + sizeof(eth::header) + sizeof(ipv4::header) +
            sizeof(udp::header));
}

#endif /* __KERNEL_NET_DHCP_H */