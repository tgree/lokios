#include "sbrk.h"
#include "kassert.h"

#define SBRK_MAX_LEN    1024*1024

extern char _sbrk[];

static void* _brk = (void*)_sbrk;
static void* _brklim = (char*)0xFFFFFFFFFFFFFFFF;

void*
kernel::sbrk(size_t n)
{
    n = (n + 15) & ~15;
    void* p = _brk;
    void* new_brk = (char*)p + n;
    kernel::kassert(new_brk <= _brklim);
    _brk = new_brk;
    return p;
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
