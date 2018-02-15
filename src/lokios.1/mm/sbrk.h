#ifndef __KERNEL_SBRK_H
#define __KERNEL_SBRK_H

#include <stddef.h>

namespace kernel
{
    void* sbrk(size_t n);
    void* get_sbrk_limit();
    void set_sbrk(void* pos);
    void set_sbrk_limit(void* new_lim);
    void freeze_sbrk();
}

#endif /* __KERNEL_SBRK_H */
