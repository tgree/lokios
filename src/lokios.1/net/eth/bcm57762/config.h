#ifndef __BCM57762_CONFIG_H
#define __BCM57762_CONFIG_H

#include "net/net.h"

// The largest possible receive MTU that the chip supports.
#define BCM57762_MAX_RX_MTU 9600

// The receive MTU we'll use for the chip.  This value does not include the
// 14-byte Ethernet header or the 4-byte Frame Checksum.
#define BCM57762_RX_MTU     1500
KASSERT(BCM57762_RX_MTU <= sizeof_field(net::rx_page,payload));
KASSERT(BCM57762_RX_MTU <= BCM57762_MAX_RX_MTU);

// The transmit MTU we'll use for the chip.  Transmitting frames larger than
// 1500-byte MTU requires setting BDs up differently and the driver currently
// doesn't support this.  So, if you set the RX MTU larger than 1500 you
// probably don't want to set the TX MTU to match otherwise things will blow up
// on large frames.
#define BCM57762_TX_MTU     1500

#endif /* __BCM57762_CONFIG_H */
