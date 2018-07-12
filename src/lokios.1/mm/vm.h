#ifndef __KERNEL_MM_VM_H
#define __KERNEL_MM_VM_H

#include <stddef.h>

namespace kernel
{
    void vmmap(void* vaddr, size_t len);
    void vmunmap(void* vaddr, size_t len);
}

#endif /* __KERNEL_MM_VM_H */
