#ifndef __KERNEL_NET_TCP_HEADER_H
#define __KERNEL_NET_TCP_HEADER_H

#include "traits.h"
#include "net/net.h"
#include "net/ip/ip.h"
#include "k++/random.h"

namespace tcp
{
    struct option_parse_exception
    {
        const char* msg;
        const uint64_t val;
        constexpr option_parse_exception(const char* msg, uint64_t val = 0):
            msg(msg),val(val) {}
    };

#define OPTION_SND_MSS_PRESENT          (1<<0)
#define OPTION_SND_WND_SHIFT_PRESENT    (1<<1)
    struct parsed_options
    {
        uint32_t    flags;
        uint32_t    snd_mss;
        uint8_t     snd_wnd_shift;
    };

    static constexpr uint16_t FFIN = (1<<0);
    static constexpr uint16_t FSYN = (1<<1);
    static constexpr uint16_t FRST = (1<<2);
    static constexpr uint16_t FPSH = (1<<3);
    static constexpr uint16_t FACK = (1<<4);
    static constexpr uint16_t FURG = (1<<5);
    static constexpr uint16_t FECE = (1<<6);
    static constexpr uint16_t FCWR = (1<<7);
    static constexpr uint16_t FNS  = (1<<8);

    struct SEQ   {uint32_t seq_num;};
    struct ACK   {uint32_t ack_num;};
    struct CTL   {uint16_t flags;};
    struct SIP   {ipv4::addr ip;};
    struct DIP   {ipv4::addr ip;};
    struct SPORT {uint16_t port;};
    struct DPORT {uint16_t port;};
    struct WS    {size_t window_size; uint8_t window_shift;};

    struct header
    {
        be_uint16_t src_port;
        be_uint16_t dst_port;
        be_uint32_t seq_num;
        be_uint32_t ack_num;
        union
        {
            struct
            {
                uint16_t    ns     : 1,
                            rsrv   : 3,
                            offset : 4,
                            fin    : 1,
                            syn    : 1,
                            rst    : 1,
                            psh    : 1,
                            ack    : 1,
                            urg    : 1,
                            ece    : 1,
                            cwr    : 1;
            };
            be_uint16_t flags_offset;
        } __PACKED__;
        be_uint16_t window_size;
        be_uint16_t checksum;
        be_uint16_t urgent_pointer;
        uint8_t options[0];

        parsed_options  parse_options() const;

        inline void _format(SEQ s)   {seq_num  = s.seq_num;}
        inline void _format(ACK a)   {ack_num  = a.ack_num;}
        inline void _format(SPORT p) {src_port = p.port;}
        inline void _format(DPORT p) {dst_port = p.port;}

        inline void _format(CTL c)
        {
            flags_offset = ((flags_offset & 0xF000) | c.flags);
        }

        inline void _format(WS ws)
        {
            kernel::kassert(ws.window_shift <= 14);
            window_size = MIN(ws.window_size >> ws.window_shift,0xFFFFUL);
        }

        template<typename T, typename ...Args>
        inline void _format(T arg0, Args... args)
        {
            _format(arg0);
            _format(args...);
        }

        template<typename ...Args>
        inline void format(Args... args)
        {
            _format(args...);
        }

        inline void _init()
        {
            src_port        = 0;
            dst_port        = 0;
            seq_num         = 0;
            ack_num         = 0;
            flags_offset    = 0x5000;
            window_size     = 0;
            checksum        = 0;
            urgent_pointer  = 0;
        }

        template<typename ...Args>
        inline void init(Args... args)
        {
            _init();
            format(args...);
        }
    } __PACKED__;
    KASSERT(sizeof(header) == 20);

    struct ipv4_tcp_headers
    {
        ipv4::header    ip;
        tcp::header     tcp;

        inline void _format(SIP i) {ip.src_ip = i.ip;}
        inline void _format(DIP i) {ip.dst_ip = i.ip;}

        template<typename T>
        inline void _format(T t)
        {
            tcp._format(t);
        }

        template<typename T, typename ...Args>
        inline void _format(T arg0, Args... args)
        {
            _format(arg0);
            _format(args...);
        }

        template<typename ...Args>
        inline void format(Args... args)
        {
            _format(args...);
        }

        inline void _init()
        {
            ip.version_ihl      = 0x45;
            ip.dscp_ecn         = 0;
            ip.total_len        = sizeof(ipv4::header) + sizeof(tcp::header);
            ip.identification   = kernel::random(0,0xFFFF);
            ip.flags_fragoffset = 0x4000;
            ip.ttl              = 64;
            ip.proto            = tcp::net_traits::ip_proto;
            ip.header_checksum  = 0;
            ip.src_ip           = ipv4::addr{0,0,0,0};
            ip.dst_ip           = ipv4::addr{0,0,0,0};
            tcp._init();
        }

        template<typename ...Args>
        inline void init(Args... args)
        {
            _init();
            format(args...);
        }

        uint32_t payload_len() const
        {
            return ip.total_len - sizeof(ipv4::header) - 4*tcp.offset;
        }

        uint32_t segment_len() const
        {
            return tcp.syn + tcp.fin + payload_len();
        }

        void* get_payload() const
        {
            return (char*)&tcp + 4*tcp.offset;
        }
    } __PACKED__;
    KASSERT(sizeof(ipv4_tcp_headers) == 40);

    // Sequence number 0 is ordered against all other sequence numbers as
    // follows:
    //      [0x80000000,...,0xFFFFFFFF, 0, ...,0x7FFFFFFF]
    //
    // Therefore, sequence number A is ordered against all other sequence
    // numers as follows:
    //      [A+0x80000000,...,A+0xFFFFFFFF, A, ...,A+0x7FFFFFFF]
    //
    // Let B be some other sequence number.  Then write B in terms of A:
    //
    //      B = A + X
    //
    // But then the comparison of A and B is simply the comparison of X and 0:
    //
    //      cmp(A,B)
    //      cmp(A,A+X)
    //      A <  B if X >  0
    //      A == B if X == 0
    //      A >  B if X <  0
    //
    // So, seq_order returns:
    //      <0 if b <  a
    //      =0 if b == a
    //      >0 if b >  a
    constexpr int32_t seq_order(uint32_t a, uint32_t b)
    {
        // TODO: This works for all cases except when a - b = 0x80000000.
        // It seems as though seq_order is actually impossible to implement
        // because there are more numbers below 0 than above 0.  That becomes
        // a problem because seq_order(A,B) != -seq_order(B,A) for all A,B.
        // An example is seen in the KASSERTs below:
        //
        //  KASSERT(seq_order(0,0x80000000) <  0);
        //  KASSERT(seq_order(0x80000000,0) <  0);
        //
        // I think this means we probably need to change the TCP stack to use
        // window comparisons instead of sequence comparisons.  See for
        // instance:
        //
        //  https://www.rfc-editor.org/ien/ien74.txt
        return (int32_t)(b - a);
    }
    KASSERT(seq_order(1,2)          >  0);
    KASSERT(seq_order(1,0xFFFFFFFF) <  0);
    KASSERT(seq_order(2,2)          == 0);
    KASSERT(seq_order(3,2)          <  0);
    KASSERT(seq_order(0xFFFFFFFF,2) >  0);
    KASSERT(seq_order(0,0x7FFFFFFF) >  0);
    KASSERT(seq_order(0,0x80000000) <  0);
    KASSERT(seq_order(0,0x80000001) <  0);
    KASSERT(seq_order(0x7FFFFFFF,0) <  0);
    KASSERT(seq_order(0x80000000,0) <  0);
    KASSERT(seq_order(0x80000001,0) >  0);

    constexpr bool seq_lt(uint32_t a, uint32_t b) {return seq_order(a,b) > 0;}
    constexpr bool seq_le(uint32_t a, uint32_t b) {return seq_order(a,b) >= 0;}
    constexpr bool seq_eq(uint32_t a, uint32_t b) {return seq_order(a,b) == 0;}
    constexpr bool seq_ne(uint32_t a, uint32_t b) {return seq_order(a,b) != 0;}
    constexpr bool seq_ge(uint32_t a, uint32_t b) {return seq_order(a,b) <= 0;}
    constexpr bool seq_gt(uint32_t a, uint32_t b) {return seq_order(a,b) < 0;}
    KASSERT(seq_lt(1,2));
    KASSERT(!seq_lt(2,2));
    KASSERT(!seq_lt(3,2));
    KASSERT(seq_le(1,2));
    KASSERT(seq_le(2,2));
    KASSERT(!seq_le(3,2));
    KASSERT(!seq_eq(1,2));
    KASSERT(seq_eq(2,2));
    KASSERT(!seq_eq(3,2));
    KASSERT(seq_ne(1,2));
    KASSERT(!seq_ne(2,2));
    KASSERT(seq_ne(3,2));
    KASSERT(!seq_ge(1,2));
    KASSERT(seq_ge(2,2));
    KASSERT(seq_ge(3,2));
    KASSERT(!seq_gt(1,2));
    KASSERT(!seq_gt(2,2));
    KASSERT(seq_gt(3,2));

    struct seq_range
    {
        uint32_t    first;
        uint32_t    len;

        constexpr bool seq_in_range(uint32_t seq) const
        {
            return (seq - first < len);
        }
    };
    constexpr bool operator==(seq_range l, seq_range r)
    {
        return l.first == r.first && l.len == r.len;
    }
    constexpr seq_range seq_bound(uint32_t first, uint32_t last)
    {
        // Converts the inclusive bound [first, last] into a seq_range.
        return seq_range{first,last-first+1};
    }
    static_assert(seq_bound(0,2).len == 3);
    static_assert(seq_bound(1,2).len == 2);
    static_assert(seq_bound(2,2).len == 1);
    static_assert(seq_bound(3,2).len == 0);
    static_assert(seq_bound(4,2).len == 0xFFFFFFFF);

    constexpr bool seq_subcheck(uint32_t rcv_nxt, uint32_t seq_num,
                                uint32_t rcv_wnd)
    {
        return seq_range{rcv_nxt,rcv_wnd}.seq_in_range(seq_num);
    }
    constexpr bool seq_check(uint32_t rcv_nxt, uint32_t seq_num, 
                             uint32_t seg_len, uint32_t rcv_wnd)
    {
        if (rcv_wnd)
        {
            if (!seg_len)
                return seq_subcheck(rcv_nxt,seq_num,rcv_wnd);
            return seq_subcheck(rcv_nxt,seq_num+seg_len-1,rcv_wnd) ||
                   seq_subcheck(rcv_nxt,seq_num,rcv_wnd);
        }
        if (!seg_len)
            return seq_num == rcv_nxt;
        return false;
    }

    constexpr seq_range seq_overlap_in_out(seq_range inner, seq_range outer)
    {
        // inner.first is contained in outer.
        return seq_range{inner.first,MIN(inner.len,outer.len - inner.first)};
    }

    constexpr seq_range seq_overlap(seq_range r1, seq_range r2)
    {
        if (r2.seq_in_range(r1.first))
            return seq_overlap_in_out(r1,r2);
        if (r1.seq_in_range(r2.first))
            return seq_overlap_in_out(r2,r1);
        return seq_range{0,0};
    }
    KASSERT(seq_overlap(seq_bound(0,10),seq_bound(1,10)) == seq_bound(1,10));
    KASSERT(seq_overlap(seq_bound(1,10),seq_bound(0,10)) == seq_bound(1,10));
    KASSERT(seq_overlap(seq_bound(0xFFFFFFF0,10),seq_bound(1,5)) ==
                        seq_bound(1,5));
    KASSERT(seq_overlap(seq_bound(1,5),seq_bound(0xFFFFFFF0,10)) ==
                        seq_bound(1,5));
    KASSERT(seq_overlap(seq_bound(1,5),seq_bound(6,10)).len == 0);
    KASSERT(seq_overlap(seq_bound(6,10),seq_bound(1,5)).len == 0);
    KASSERT(seq_overlap(seq_bound(0xFFFFFFF0,10),seq_bound(11,15)).len == 0);
    KASSERT(seq_overlap(seq_bound(11,15),seq_bound(0xFFFFFFF0,10)).len == 0);
    KASSERT(seq_overlap(seq_range{10,0},seq_bound(10,5)).len == 0);

    uint16_t compute_checksum(net::tx_op* op, size_t llhdr_size);
}

#endif /* __KERNEL_NET_TCP_HEADER_H */
