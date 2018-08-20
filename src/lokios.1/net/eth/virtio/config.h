#ifndef __KERNEL_NET_VIRTIO_CONFIG_H
#define __KERNEL_NET_VIRTIO_CONFIG_H

// Whether or not to hexdump packets to the kernel console on the TX path.
#define VIRTIO_NET_DUMP_TX_PACKETS  0

// The receive MTU.
#define VIRTIO_NET_RX_MTU           1500

// The transmit MTU.
#define VIRTIO_NET_TX_MTU           1500

#endif /* __KERNEL_NET_VIRTIO_CONFIG_H */
