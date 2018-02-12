#ifndef __KERNEL_MM_VM_H
#define __KERNEL_MM_VM_H

namespace kernel
{
    enum vm_region_type
    {
        VM_REGION_RSRVD     = 0,
        VM_REGION_FREE      = 1,
        VM_REGION_MAPPED    = 2,
    };

    struct vm_region
    {
        kdlink      all_link;
        kdlink      free_link;
        uint64_t    vpfn;
        uint64_t    npfns;
        uint64_t    rsrv[2];
    };
    KASSERT(sizeof(vm_region) == 64);

#define VMA_FLAG_GUARDED    (1<<0)
#define VMA_FLAG_POPULATE   (1<<1)
    void*   vm_alloc(size_t npfns,
                     uint64_t flags = VMA_FLAG_GUARDED | VMA_FLAG_POPULATE,
                     uint64_t alignment = 1);
    void    vm_free(void* p);
}

#endif /* __KERNEL_MM_VM_H */
