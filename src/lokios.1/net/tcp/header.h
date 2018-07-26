#ifndef __KERNEL_NET_TCP_HEADER_H
#define __KERNEL_NET_TCP_HEADER_H

#include "net/net.h"

namespace tcp
{
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
    } __PACKED__;
    KASSERT(sizeof(header) == 20);

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
    //      cmp(0,X)
    //      A <  B if X <  0
    //      A == B if X == 0
    //      A >  B if X >  0
    //
    // So, seq_order returns:
    //      <0 if b <  a
    //      =0 if b == a
    //      >0 if b >  a
    constexpr int32_t seq_order(uint32_t a, uint32_t b)
    {
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

    constexpr bool seq_subcheck(uint32_t rcv_nxt, uint32_t seq_num,
                                uint32_t rcv_wnd)
    {
        return seq_le(rcv_nxt,seq_num) && seq_lt(seq_num,rcv_nxt+rcv_wnd);
    }
    constexpr bool seq_check(uint32_t rcv_nxt, uint32_t seq_num, 
                             uint32_t seg_len, uint32_t rcv_wnd)
    {
        return seq_subcheck(rcv_nxt,seq_num,rcv_wnd) ||
               seq_subcheck(rcv_nxt,seq_num+seg_len-1,rcv_wnd);
    }

    uint16_t compute_checksum(net::tx_op* op, size_t llhdr_size);
}

#endif /* __KERNEL_NET_TCP_HEADER_H */
