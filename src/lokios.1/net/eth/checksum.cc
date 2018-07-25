#include "checksum.h"
#include "net/eth/header.h"
#include "net/ip/ip.h"
#include "net/udp/udp.h"
#include "net/tcp/header.h"

using kernel::_kassert;

void
eth::insert_checksums(net::tx_op* op)
{
    kassert(op->nalps > 0);
    kassert(!((op->flags & NTX_FLAG_INSERT_TCP_CSUM) &&
              (op->flags & NTX_FLAG_INSERT_UDP_CSUM)));

    auto* llhdr = (eth::header*)kernel::phys_to_virt(op->alps[0].paddr);
    auto* iphdr = (ipv4::header*)(llhdr + 1);
    auto* uhdr  = (udp::header*)(iphdr + 1);
    auto* thdr  = (tcp::header*)(iphdr + 1);
    if (op->flags & NTX_FLAG_INSERT_TCP_CSUM)
        thdr->checksum = tcp::compute_checksum(op,sizeof(eth::header));
    else if (op->flags & NTX_FLAG_INSERT_UDP_CSUM)
        uhdr->checksum = udp::compute_checksum(op,sizeof(eth::header));
    if (op->flags & NTX_FLAG_INSERT_IP_CSUM)
    {
        kassert(op->alps[0].len >= sizeof(eth::header) + sizeof(ipv4::header));
        kassert(iphdr->header_checksum == 0);
        iphdr->header_checksum = ipv4::csum(iphdr);
    }
}
