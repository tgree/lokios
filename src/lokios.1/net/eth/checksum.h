#ifndef __KERNEL_NET_ETH_CHECKSUM_H
#define __KERNEL_NET_ETH_CHECKSUM_H

#include "net/net.h"

namespace eth
{
    void insert_checksums(net::tx_op* op);
}

#endif /* __KERNEL_NET_ETH_CHECKSUM_H */
