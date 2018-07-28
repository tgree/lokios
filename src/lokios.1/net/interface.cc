#include "interface.h"
#include "kernel/console.h"
#include "mm/vm.h"
#include "k++/kmath.h"

using kernel::_kassert;

static kernel::spinlock ids_lock;
static uint32_t free_ids = 0xFFFFFFFF;

static size_t
alloc_id()
{
    with (ids_lock)
    {
        kassert(free_ids != 0);
        size_t id = kernel::ffs(free_ids);
        free_ids &= ~(1<<id);
        return id;
    }
}

static void
free_id(size_t id)
{
    with (ids_lock)
    {
        free_ids |= (1<<id);
    }
}

net::interface::interface():
    id(alloc_id()),
    intf_mem((interface_mem*)(MM_ETH_BEGIN + id*MM_ETH_STRIDE)),
    ip_addr{0,0,0,0}
{
    kernel::vmmap(intf_mem,sizeof(*intf_mem));
    memset(intf_mem,0,sizeof(*intf_mem));
    new(intf_mem) interface_mem();
}

net::interface::~interface()
{
    intf_mem->~interface_mem();
    kernel::vmunmap(intf_mem,sizeof(*intf_mem));
    free_id(id);
}

void
net::interface::intf_vdbg(const char* fmt, va_list ap)
{
    kernel::console::p2printf(fmt,ap,"net%zu: ",id);
}
