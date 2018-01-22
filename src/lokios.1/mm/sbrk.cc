#include "sbrk.h"
#include "kernel/spinlock.h"
#include "kernel/kassert.h"

#define SBRK_MAX_LEN    1024*1024

extern char _sbrk[];

static kernel::spinlock sbrklock;
static void* _brk = (void*)_sbrk;
static void* _brklim = (char*)0xFFFFFFFFFFFFFFFF;

void*
kernel::sbrk(size_t n)
{
    void* p;
    void* new_brk;

    n = (n + 15) & ~15;
    with (sbrklock)
    {
        p    = _brk;
        _brk = new_brk = (char*)p + n;
    }
    kernel::kassert(new_brk <= _brklim);
    return p;
}

void
kernel::set_sbrk(void* pos)
{
    _brk = pos;
}

void
kernel::set_sbrk_limit(void* new_lim)
{
    kernel::kassert(new_lim <= _brklim);
    _brklim = new_lim;
}

void*
kernel::get_sbrk_limit()
{
    return _brklim;
}
