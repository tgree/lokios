#ifndef __KERNEL_NET_VIRTIO_QUEUE_H
#define __KERNEL_NET_VIRTIO_QUEUE_H

#include "mm/mm.h"

namespace virtio_net
{
#define VIRTQ_DESC_F_NEXT       (1<<0)
#define VIRTQ_DESC_F_WRITE      (1<<1)
#define VIRTQ_DESC_F_INDIRECT   (1<<2)
    struct descriptor
    {
        kernel::dma_addr64  addr;
        uint32_t            len;
        uint16_t            flags;
        uint16_t            next;
    };
    KASSERT(sizeof(descriptor) == 16);

#define VIRTQ_AVAIL_F_NO_INTERRUPT  (1<<0)
    struct available_ring
    {
        volatile uint16_t    flags;
        volatile uint16_t    idx;
        volatile uint16_t    ring[];
    };

    struct used_elem
    {
        uint32_t    id;
        uint32_t    len;
    };

#define VIRTQ_USED_F_NO_NOTIFY  (1<<0)
    struct used_ring
    {
        volatile uint16_t    flags;
        volatile uint16_t    idx;
        volatile used_elem   ring[];
    };

    struct vqueue
    {
        const size_t                index;
        uint16_t                    size;
        uint16_t                    size_mask;
        uint16_t                    free_descriptors_head;
        uint16_t                    used_pos;

        struct descriptor*          descriptors;
        struct used_ring*           used_ring;
        struct available_ring*      avail_ring;
        uint16_t*                   notify_addr;

        // Initialize the queue to the given size.
        void init(size_t size, uint16_t* notify_addr);

        // Allocates and frees chains of descriptors.  The next fields in the
        // descriptors are linked upon allocation and consulted upon
        // deallocation, so the caller shouldn't manipulate those at all.
        uint16_t alloc_descriptors(size_t n);
        void free_descriptors(uint16_t index);
        uint16_t get_free_descriptor_count();
        
        vqueue(size_t index);
        ~vqueue();
    };

}

#endif /* __KERNEL_NET_VIRTIO_QUEUE_H */
