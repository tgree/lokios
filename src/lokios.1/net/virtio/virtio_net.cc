/*
 * Driver for the virtio network device.
 * Reference:
 *  http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.pdf
 *  http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html
 *
 * To make this work in qemu you'll need something like the following options:
 *  -device virtio-net-pci,netdev=net0,disable-legacy=on,disable-modern=off, \
 *          vectors=4
 *  -netdev user,id=net0
 */
#include "virtio_net.h"
#include "virtio_interface.h"
#include "kernel/cpu.h"
#include "kernel/console.h"
#include "../eth.h"
#include "../udp.h"
#include "../dhcp.h"

using kernel::console::printf;
using kernel::_kassert;
using kernel::virt_to_phys;
using kernel::phys_to_virt;

virtio_net::dev::dev(const kernel::pci::dev* pd,
    const virtio_net::driver* owner):
        kernel::pci::dev(pd,owner),
        state(VN_STATE_IDLE),
        intf(NULL),
        rq(0),
        tq(1),
        cq(2)
{
    // Set up the timer wqe.
    timer_wqe.fn      = timer_delegate(handle_timer);
    timer_wqe.args[0] = (uintptr_t)this;

    // Populate register pointers.
    common_cfg = NULL;
    device_cfg = NULL;
    notify_cfg = NULL;
    for (uint8_t pos : cap_list())
    {
        uint8_t cap_id = config_read_8(pos);
        if (cap_id != 0x09)
            continue;

        uint8_t cap_len = config_read_8(pos + 2);
        if (cap_len < sizeof(virtio_net::pci_cap))
            continue;

        uint8_t cap_type = config_read_8(pos + 3);
        uint8_t bar      = config_read_8(pos + 4);
        uint32_t offset  = config_read_32(pos + 8);
        uint32_t len     = config_read_32(pos + 12);
        switch (cap_type)
        {
            case VIRTIO_PCI_CAP_COMMON_CFG:
                common_cfg = (struct common_cfg*)map_bar(bar,len,offset);
            break;

            case VIRTIO_PCI_CAP_NOTIFY_CFG:
                kassert(cap_len >= sizeof(virtio_net::pci_cap) + 4);
                notify_cfg = map_bar(bar,len,offset);
                notify_offset_multiplier = config_read_32(pos + 16);
            break;

            case VIRTIO_PCI_CAP_DEVICE_CFG:
                device_cfg = (net_config*)map_bar(bar,len,offset);
            break;
        }
    }
    kassert(common_cfg != NULL);

    // Start by resetting the device.
    issue_reset();
    arm_timer(1);
    state = VN_STATE_WAIT_RESET;
}

uint64_t
virtio_net::dev::get_device_features()
{
    common_cfg->device_feature_select = 1;
    uint64_t f = ((uint64_t)common_cfg->device_feature << 32);
    common_cfg->device_feature_select = 0;
    return f | common_cfg->device_feature;
}

void
virtio_net::dev::set_driver_features(uint64_t f)
{
    common_cfg->driver_feature_select = 1;
    common_cfg->driver_feature = (f >> 32);
    common_cfg->driver_feature_select = 0;
    common_cfg->driver_feature = f;
}

uint16_t*
virtio_net::dev::get_queue_notify_addr()
{
    return (uint16_t*)((uintptr_t)notify_cfg +
               common_cfg->queue_notify_off*notify_offset_multiplier);
}

void
virtio_net::dev::arm_timer(uint64_t dt10ms)
{
    kernel::cpu::schedule_timer(&timer_wqe,dt10ms);
}

void
virtio_net::dev::issue_reset()
{
    common_cfg->device_status = 0;
}

void
virtio_net::dev::issue_set_mac_address()
{
    // Send a command to the chip.
    auto* cmd                   = (set_mac_addr_cmd*)kernel::page_alloc();
    cmd->cls                    = 1;
    cmd->command                = 1;
    cmd->mac                    = mac;
    cmd->ack                    = 0xFF;

    // Allocate some descriptors.
    uint16_t dhead = cq.alloc_descriptors(2);
    kassert(dhead != 0xFFFF);

    // First descriptor contains the set_mac_addr_cmd address.
    descriptor* d = &cq.descriptors[dhead];
    d->addr       = kernel::virt_to_phys(cmd);
    d->len        = sizeof(*cmd) - 1;
    kassert(d->flags & VIRTQ_DESC_F_NEXT);
    d->flags      = VIRTQ_DESC_F_NEXT;

    // Second descriptor is the ack byte that comes back from the device.
    d             = &cq.descriptors[d->next];
    d->addr       = kernel::virt_to_phys((void*)&cmd->ack);
    d->len        = 1;
    kassert(!(d->flags & VIRTQ_DESC_F_NEXT));
    d->flags = VIRTQ_DESC_F_WRITE;

    // Post it to the queue.
    cq.push(dhead);
}

void
virtio_net::dev::post_tx_frame(eth::tx_op* op)
{
    // Allocate some descriptors.
    uint16_t dhead = tq.alloc_descriptors(op->nalps + 1);
    kassert(dhead < kernel::nelems(tx_table));
    tx_table[dhead] = op;

    // First descriptor is 12-bytes of zeroes.
    descriptor* d = &tq.descriptors[dhead];
    d->addr       = kernel::zero_page_dma;
    d->len        = sizeof(net_hdr);

    // Do all the payload alps.
    for (size_t i=0; i<op->nalps; ++i)
    {
        kassert(d->flags & VIRTQ_DESC_F_NEXT);
        d->flags = VIRTQ_DESC_F_NEXT;
        d        = &tq.descriptors[d->next];
        d->addr  = op->alps[i].paddr;
        d->len   = op->alps[i].len;
    }

    // Last descriptor shouldn't have a next link.
    kassert(!(d->flags & VIRTQ_DESC_F_NEXT));
    d->flags = 0;

    // Post it to the queue.
    TODO("Ring overflow");
    tq.push(dhead);
}

void
virtio_net::dev::post_rx_pages(kernel::klist<eth::rx_page>& pages)
{
    size_t count = 0;

    TODO("Ring overflow");
    while (!pages.empty())
    {
        // Get the rx_op and make it into an rx_frame.
        auto* p = klist_front(pages,link);
        pages.pop_front();

        // Allocate the descriptor.
        uint16_t dhead = rq.alloc_descriptors(1);
        kassert(dhead != 0xFFFF);

        // Fill it out.
        descriptor* d    = &rq.descriptors[dhead];
        d->addr          = kernel::virt_to_phys(p->payload);
        d->len           = sizeof(p->payload);
        d->flags         = VIRTQ_DESC_F_WRITE;

        // Post it to the queue.
        rq.post(count,dhead);
        ++count;
    }

    // Notify the queue.
    rq.notify_posted(count);
}

void
virtio_net::dev::handle_timer(kernel::timer_entry*)
{
    switch (state)
    {
        case VN_STATE_WAIT_RESET:
        {
            // Make sure it really reset.
            kassert(common_cfg->device_status == 0);

            // ACK that we have seen the device and know how to drive it.
            common_cfg->device_status = 0x01;
            common_cfg->device_status = 0x03;

            // Set the features we want.
            uint64_t dev_features = get_device_features();
            kassert(dev_features & VIRTIO_NET_F_MAC);
            kassert(dev_features & VIRTIO_NET_F_CTRL_VQ);
            kassert(dev_features & VIRTIO_NET_F_CTRL_RX);
            kassert(dev_features & VIRTIO_NET_F_CTRL_MAC_ADDR);
            kassert(dev_features & VIRTIO_F_VERSION_1);
            set_driver_features(VIRTIO_NET_F_MAC |
                                VIRTIO_NET_F_CTRL_VQ |
                                VIRTIO_NET_F_CTRL_RX |
                                VIRTIO_NET_F_CTRL_MAC_ADDR |
                                VIRTIO_F_VERSION_1);

            // Feature negotiation complete.
            common_cfg->device_status = 0xB;
            kassert(common_cfg->device_status & 8);
            kassert(!(common_cfg->device_status & 64));

            // Configure and enable individual MSI-X vectors.
            alloc_msix_vector(rq.index,work_delegate(handle_rq_dsr));
            alloc_msix_vector(tq.index,work_delegate(handle_tq_dsr));
            alloc_msix_vector(cq.index,work_delegate(handle_cq_dsr));
            alloc_msix_vector(3,work_delegate(handle_config_change_dsr));
            for (size_t i=0; i<4; ++i)
                enable_msix_vector(i);

            // Configure the PCI command register to enable bus mastering and
            // disable INTx interrupts.
            config_write_16(0x0507,0x04);

            // Set the config msix vector.
            common_cfg->msix_config = 3;

            // Set up the queues.  Note that we have to set the enable flag
            // last because that triggers the device to fetch the other fields
            // and cache them.  Or something.
            for (auto* vq : {&rq,&tq,&cq})
            {
                common_cfg->queue_select = vq->index;

                // We don't support queue sizes larger than 256 since that
                // would require multiple descriptor pages.
                if (common_cfg->queue_size > 256)
                    common_cfg->queue_size = 256;

                vq->init(common_cfg->queue_size,get_queue_notify_addr());
                kassert(vq->get_free_descriptor_count() == vq->size);
                common_cfg->queue_msix_vector = vq->index;
                common_cfg->queue_desc        = virt_to_phys(vq->descriptors);
                common_cfg->queue_avail       = virt_to_phys(vq->avail_ring);
                common_cfg->queue_used        = virt_to_phys(vq->used_ring);
                common_cfg->queue_enable      = 1;
            }
            
            // Ready!
            common_cfg->device_status = 0x0F;
            kassert(common_cfg->device_status == 0x0F);

            // Fetch the MAC address.
            mac = device_cfg->mac;
            printf("virtio_net: MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                   mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

            // Do a dummy MAC set command to test interrupts and queueing.
            issue_set_mac_address();
            state = VN_STATE_WAIT_SET_MAC_COMP;
        }
        break;

        case VN_STATE_IDLE:
        case VN_STATE_WAIT_SET_MAC_COMP:
        case VN_STATE_DONE:
            kernel::panic("virtio_net: Unexpected timeout");
        break;
    }
}

void
virtio_net::dev::handle_rq_dsr(kernel::work_entry*)
{
    // Handle completed RQ buffers from the used_ring.  The id field of the
    // used_elem struct is equal to the value that we pushed into the
    // avail_ring when we posted the receive buffer, i.e. it will be equal to
    // the index of the descriptor at the head of the buffer's chain.
    kernel::klist<eth::rx_page> pages;
    while (!rq.empty())
    {
        auto ue = rq.pop();
        printf("virtio_net: rq completion descriptor %u len %u\n",ue.id,ue.len);

        auto* d       = &rq.descriptors[ue.id];
        auto* p       = container_of((uint8_t*)phys_to_virt(d->addr),
                                     eth::rx_page,payload[0]);
        p->eth_len    = ue.len - sizeof(net_hdr);
        p->eth_offset = sizeof(net_hdr);
        pages.push_back(&p->link);
        rq.free_descriptors(ue.id);
    }
    intf->handle_rx_pages(pages);

    enable_msix_vector(rq.index);
}

void
virtio_net::dev::handle_tq_dsr(kernel::work_entry*)
{
    // Handle completed TQ buffers from the used_ring.  The id field of the
    // used_elem struct is equal to the value that we pushed into the
    // avail_ring when we posted the transmit buffer, i.e. it will be equal to
    // the index of the descriptor at the head of the buffer's chain.
    while (!tq.empty())
    {
        auto ue = tq.pop();
        printf("virtio_net: tq completion descriptor %u len %u\n",ue.id,ue.len);
        intf->handle_tx_completion(tx_table[ue.id]);
        tq.free_descriptors(ue.id);
    }

    enable_msix_vector(tq.index);
}

void
virtio_net::dev::handle_cq_dsr(kernel::work_entry*)
{
    while (!cq.empty())
    {
        auto ue = cq.pop();
        printf("virtio_net: cq completion descriptor %u len %u\n",ue.id,ue.len);
        handle_cq_completion(ue.id);
        cq.free_descriptors(ue.id);
    }

    enable_msix_vector(cq.index);
}

void
virtio_net::dev::handle_cq_completion(uint16_t index)
{
    switch (state)
    {
        case VN_STATE_WAIT_SET_MAC_COMP:
        {
            auto* cmd = (virtio_net::set_mac_addr_cmd*)
                phys_to_virt(cq.descriptors[index].addr);
            kassert(cmd->ack == 0);
            kassert(cq.used_ring->idx == 1);
            printf("virtio_net: set mac address completed\n");

            // Create the interface.
            intf = new virtio_net::interface(this);
            intf->activate();

            state = VN_STATE_DONE;
        }
        break;

        case VN_STATE_IDLE:
        case VN_STATE_WAIT_RESET:
        case VN_STATE_DONE:
            kernel::panic("virtio_net: Unexpected cq dsr");
        break;
    }
}

void
virtio_net::dev::handle_config_change_dsr(kernel::work_entry*)
{
    printf("virtio_net: config change dsr\n");
}
