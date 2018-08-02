#ifndef __KERNEL_KBACKTRACE_H
#define __KERNEL_KBACKTRACE_H

#include <stdint.h>
#include <stddef.h>

namespace kernel
{
    void backtrace(uintptr_t* pcs, size_t npcs);

    void init_backtrace();
}

#endif /* __KERNEL_KBACKTRACE_H */
