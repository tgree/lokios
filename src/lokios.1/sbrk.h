#ifndef __KERNEL_SBRK_H
#define __KERNEL_SBRK_H

#include <stddef.h>

namespace kernel
{
    void* sbrk(size_t n);
    void* get_sbrk_limit();
    void set_sbrk_limit(void* new_lim);
}

#endif /* __KERNEL_SBRK_H */
