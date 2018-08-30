#ifndef __KERNEL_KBACKTRACE_H
#define __KERNEL_KBACKTRACE_H

#include <stdint.h>
#include <stddef.h>

namespace kernel
{
    void backtrace(const char* header = NULL);
}

#endif /* __KERNEL_KBACKTRACE_H */
