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

tcp::parsed_options
tcp::header::parse_options() const
{
    parsed_options popts;
    size_t rem         = offset*4 - sizeof(tcp::header);
    const uint8_t* opt = options;
    popts.flags        = 0;

    while (rem)
    {
        switch (*opt)
        {
            case 0:     // End-of-options.
                rem = 0;
            break;

            case 1:     // No-op.
                --rem;
                ++opt;
            break;

            case 2:     // MSS value.
                if (rem < 4)
                    throw option_parse_exception("truncated MSS option");
                if (opt[1] != 4)
                    throw option_parse_exception("bad MSS len",opt[1]);
                popts.flags  |= OPTION_SND_MSS_PRESENT;
                popts.snd_mss = *(be_uint16_t*)(opt + 2);
                rem -= 4;
                opt += 4;
            break;

            case 3:     // Window Size Shift
                if (rem < 3)
                    throw option_parse_exception("truncated WND_SHIFT option");
                if (opt[1] != 3)
                    throw option_parse_exception("bad WND_SHIFT len",opt[1]);
                popts.flags        |= OPTION_SND_WND_SHIFT_PRESENT;
                popts.snd_wnd_shift = opt[2];
                if (popts.snd_wnd_shift > 14)
                    popts.snd_wnd_shift = 14;
                rem -= 3;
                opt += 3;
            break;

            default:    // Anything else.
                if (rem < 2)
                    throw option_parse_exception("option missing len",opt[0]);
                if (rem < opt[1])
                    throw option_parse_exception("option truncated",opt[0]);
                rem -= opt[1];
                opt += opt[1];
            break;
        }
    }

    return popts;
}

size_t
tcp::header::append_options(parsed_options opts)
{
    auto* opt = options;

    if (opts.flags & OPTION_SND_MSS_PRESENT)
    {
        opt[0]                   = 2;
        opt[1]                   = 4;
        *(be_uint16_t*)(opt + 2) = opts.snd_mss;
        opt                     += 4;
    }

    if (opts.flags & OPTION_SND_WND_SHIFT_PRESENT)
    {
        opt[0] = 3;
        opt[1] = 3;
        opt[2] = opts.snd_wnd_shift;
        opt[3] = 1;
        opt   += 4;
    }

    size_t opt_len = opt - options;
    offset += opt_len/4;
    return opt_len;
}
