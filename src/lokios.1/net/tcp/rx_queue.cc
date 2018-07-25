#include "rx_queue.h"

using kernel::_kassert;

tcp::rx_queue::rx_queue(kernel::delegate<void(rx_queue*)> rq_ready_delegate):
    avail_bytes(0),
    rq_ready_delegate(rq_ready_delegate)
{
}

void
tcp::rx_queue::append(net::rx_page* p)
{
    pages.push_back(&p->link);
    avail_bytes += p->client_len;
    rq_ready_delegate(this);
}

void
tcp::rx_queue::read(void* _dst, uint32_t rem)
{
    kassert(rem <= avail_bytes);
    avail_bytes -= rem;

    char* dst = (char*)_dst;
    while (rem)
    {
        net::rx_page* p = klist_front(pages,link);
        uint32_t len    = kernel::min(rem,(uint32_t)p->client_len);
        memcpy(dst,p->payload + p->client_offset,len);
        p->client_offset += len;
        p->client_len    -= len;
        dst              += len;
        rem              -= len;

        if (!p->client_len)
        {
            pages.pop_front();
            delete p;
        }
    }
}
