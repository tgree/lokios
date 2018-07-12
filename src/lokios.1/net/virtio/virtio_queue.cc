#include "virtio_queue.h"
#include "mm/page.h"

virtio_net::vqueue::vqueue(size_t index):
    index(index),
    size(0),
    size_mask(0),
    free_descriptors_head(-1),
    used_pos(0),
    notify_addr(NULL)
{
}

virtio_net::vqueue::~vqueue()
{
}

void
virtio_net::vqueue::init(size_t _size, uint16_t* _notify_addr)
{
    size        = _size;
    size_mask   = size - 1;
    notify_addr = _notify_addr;

    for (size_t i=0; i<size; ++i)
        free_descriptors(i);
}

uint16_t
virtio_net::vqueue::alloc_descriptors(size_t n)
{
    if (!n || free_descriptors_head == 0xFFFF)
        return 0xFFFF;

    uint16_t index;
    uint16_t head;
    index = head = free_descriptors_head;
    while (--n)
    {
        if (!(descriptors[index].flags & VIRTQ_DESC_F_NEXT))
            return 0xFFFF;

        index = descriptors[index].next;
    }

    if (descriptors[index].flags & VIRTQ_DESC_F_NEXT)
        free_descriptors_head = descriptors[index].next;
    else
        free_descriptors_head = 0xFFFF;

    descriptors[index].flags = 0;
    return head;
}

void
virtio_net::vqueue::free_descriptors(uint16_t index)
{
    uint16_t old_head      = free_descriptors_head;
    free_descriptors_head  = index;
    if (old_head == 0xFFFF)
        return;

    while (descriptors[index].flags & VIRTQ_DESC_F_NEXT)
        index = descriptors[index].next;
    descriptors[index].next  = old_head;
    descriptors[index].flags = VIRTQ_DESC_F_NEXT;
}

uint16_t
virtio_net::vqueue::get_free_descriptor_count()
{
    uint16_t index = free_descriptors_head;
    if (index == 0xFFFF)
        return 0;

    size_t n = 1;
    while (descriptors[index].flags & VIRTQ_DESC_F_NEXT)
    {
        index = descriptors[index].next;
        ++n;
    }
    return n;
}
