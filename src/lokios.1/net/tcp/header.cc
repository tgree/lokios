#include "header.h"
#include "traits.h"
#include "net/ip/ip.h"

using kernel::_kassert;

struct tcp_ip_headers
{
    ipv4::header    iphdr;
    tcp::header     thdr;
} __PACKED__;

struct pseudo_header
{
    ipv4::addr  src_ip;
    ipv4::addr  dst_ip;
    be_uint16_t len;
    uint8_t     zero;
    uint8_t     proto;
} __PACKED__;
KASSERT(sizeof(pseudo_header) == 12);

uint16_t
tcp::compute_checksum(net::tx_op* op, size_t llhdr_size)
{
    kassert(op->alps[0].len >= llhdr_size + sizeof(tcp_ip_headers));

    auto* hdrs = (tcp_ip_headers*)kernel::phys_to_virt(op->alps[0].paddr +
                                                       llhdr_size);

    kassert(hdrs->iphdr.proto == tcp::net_traits::ip_proto);
    kassert(hdrs->thdr.checksum == 0);

    pseudo_header phdr;
    phdr.src_ip = hdrs->iphdr.src_ip;
    phdr.dst_ip = hdrs->iphdr.dst_ip;
    phdr.len    = hdrs->iphdr.total_len - sizeof(ipv4::header);
    phdr.proto  = tcp::net_traits::ip_proto;
    phdr.zero   = 0;

    uint16_t csum = net::ones_comp_csum(&phdr,sizeof(phdr),0);
    size_t offset = sizeof(phdr);

    size_t alp0_len = op->alps[0].len - llhdr_size - sizeof(ipv4::header);
    csum = net::ones_comp_csum(&hdrs->thdr,alp0_len,offset,csum);
    offset += alp0_len;
    for (size_t i=1; i<op->nalps; ++i)
    {
        csum = net::ones_comp_csum(kernel::phys_to_virt(op->alps[i].paddr),
                                   op->alps[i].len,offset,csum);
        offset += op->alps[i].len;
    }

    return csum;
}
