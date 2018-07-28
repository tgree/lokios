#include "../interface.h"
#include "k++/kmath.h"
#include <tmock/tmock.h>

static uint32_t free_ids = 0xFFFFFFFF;

static size_t
alloc_id()
{
    kernel::kassert(free_ids != 0);
    size_t id = kernel::ffs(free_ids);
    free_ids &= ~(1<<id);
    return id;
}

static void
free_id(size_t id)
{
    free_ids |= (1<<id);
}

net::interface::interface():
    id(alloc_id()),
    intf_mem((net::interface_mem*)malloc(sizeof(*intf_mem))),
    ip_addr{0,0,0,0}
{
}

net::interface::~interface()
{
    free_id(id);
}

void
net::interface::intf_vdbg(const char* fmt, va_list ap)
{
    mock("net::interface::intf_vdbg",fmt,ap);
}
