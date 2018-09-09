#include "vm.h"
#include "page.h"
#include "buddy_allocator.h"
#include "kern/cpu.h"

void
kernel::vmmap(void* _vaddr, size_t len)
{
    uintptr_t vaddr = (uintptr_t)_vaddr;
    len             = round_up_pow2(len,PAGE_SIZE);
    kassert((vaddr & PAGE_OFFSET_MASK) == 0);

    bool gp = (get_current_cpu()->flags & CPU_FLAG_PAGESIZE_1G);
    size_t rem_pfns = len/PAGE_SIZE;
    while (rem_pfns)
    {
        size_t npfns;
        dma_addr64 paddr = -1;
        if (paddr == -1UL && gp && rem_pfns >= 512*512 &&
            !(vaddr & GPAGE_OFFSET_MASK))
        {
            try
            {
                paddr = virt_to_phys(buddy_alloc_by_len(512*512*PAGE_SIZE));
                npfns = 512*512;
            }
            catch (buddy_allocator_oom_exception&)
            {
            }
        }
        if (paddr == -1UL && rem_pfns >= 512 && !(vaddr & HPAGE_OFFSET_MASK))
        {
            try
            {
                paddr = virt_to_phys(buddy_alloc_by_len(512*PAGE_SIZE));
                npfns = 512;
            }
            catch (buddy_allocator_oom_exception&)
            {
            }
        }
        if (paddr == -1UL)
        {
            paddr = virt_to_phys(buddy_alloc_by_len(PAGE_SIZE));
            npfns = 1;
        }
        mmap((void*)vaddr,paddr,PAGE_SIZE*npfns,PAGE_FLAGS_DATA);

        vaddr    += PAGE_SIZE*npfns;
        rem_pfns -= npfns;
    }
}

void
kernel::vmunmap(void* vaddr, size_t len)
{
    // TODO: Ummmm.. this isn't freeing any of the buddy pages.
    munmap(vaddr,len);
}
