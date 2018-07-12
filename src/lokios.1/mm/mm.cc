#include "mm.h"
#include "page_table.h"
#include "sbrk.h"
#include "../x86.h"
#include "../console.h"
#include "../task.h"
#include "../cpu.h"
#include "../tlb.h"

using kernel::console::printf;

dma_addr64 kernel::zero_page_dma;
dma_addr64 kernel::trash_page_dma;
static uintptr_t top_addr;

void
kernel::preinit_mm(const e820_map* m)
{
    // Walk the page tables to find the last mapped address.
    for (const auto pte : page_table_leaf_iterator(mfcr3()))
        top_addr = pte.get_paddr() + pte.get_len();
    printf("End of bootloader-mapped RAM: 0x%016lX\n",top_addr);

    // Pre-initialize the page list.
    page_preinit(m,top_addr);
    
    // Print out the sbrk stats.
    printf("  Free sbrk RAM: %luK\n",
           ((uint64_t)get_sbrk_limit() - (uint64_t)get_sbrk())/1024);

    // Set up the zero/trash pages.
    zero_page_dma  = virt_to_phys(page_zalloc());
    trash_page_dma = virt_to_phys(page_zalloc());
}

void
kernel::init_mm(const e820_map* m)
{
    // Post-init the remaining usable pages.
    page_init(m,top_addr);

    // Print out the page stats.
    size_t free_pages = page_count_free();
    printf("Free mapped RAM: %zuMB (%zu 4K pages)\n",free_pages/256,free_pages);
}

void*
kernel::pmap(dma_addr64 paddr, uint64_t flags)
{
    uint64_t paddr_4k = round_down_pow2(paddr,PAGE_SIZE);
    uint64_t vaddr_4k = 0xFFFF800000000000UL | paddr_4k;
    kernel_task->pt.map_4k_page((void*)vaddr_4k,paddr_4k,flags);
    return (void*)(vaddr_4k | (paddr & PAGE_OFFSET_MASK));
}

void*
kernel::pmap_range(dma_addr64 paddr, size_t len, uint64_t flags)
{
    uint64_t start = round_down_pow2(paddr,PAGE_SIZE);
    uint64_t end   = round_up_pow2(paddr + len,PAGE_SIZE);
    void* vaddr    = pmap(paddr,flags);
    for (uint64_t p = start + PAGE_SIZE; p != end; p += PAGE_SIZE)
        pmap(p,flags);
    return vaddr;
}

void
kernel::mmap(void* _vaddr, dma_addr64 paddr, size_t len, uint64_t flags)
{
    uintptr_t vaddr = (uintptr_t)_vaddr;
    kassert((vaddr & PAGE_OFFSET_MASK) == 0);
    kassert((len & PAGE_OFFSET_MASK) == 0);

    bool gp = (get_current_cpu()->flags & CPU_FLAG_PAGESIZE_1G);
    size_t rem_pfns = len/PAGE_SIZE;
    while (rem_pfns)
    {
        size_t npfns;
        if (gp && rem_pfns >= 512*512 &&
            !(paddr & GPAGE_OFFSET_MASK) && !(vaddr & GPAGE_OFFSET_MASK))
        {
            kernel_task->pt.map_1g_page((void*)vaddr,paddr,flags);
            npfns = 512*512;
        }
        else if(rem_pfns >= 512 &&
                !(paddr & HPAGE_OFFSET_MASK) && !(vaddr & HPAGE_OFFSET_MASK))
        {
            kernel_task->pt.map_2m_page((void*)vaddr,paddr,flags);
            npfns = 512;
        }
        else
        {
            kernel_task->pt.map_4k_page((void*)vaddr,paddr,flags);
            npfns = 1;
        }
        
        vaddr    += PAGE_SIZE*npfns;
        paddr    += PAGE_SIZE*npfns;
        rem_pfns -= npfns;
    }
}

void
kernel::munmap(void* _vaddr, size_t len)
{
    uintptr_t vaddr = (uintptr_t)_vaddr;
    kassert((len & PAGE_OFFSET_MASK) == 0);
    kassert((vaddr & PAGE_OFFSET_MASK) == 0);
    while (len)
    {
        size_t s = kernel_task->pt.unmap_page((void*)vaddr);
        kassert(s <= len);
        len   -= s;
        vaddr += s;
    }
    kernel::tlb_shootdown();
}

dma_addr64
kernel::virt_to_phys(const void* v)
{
    kassert(v != NULL);
    kassert(((uintptr_t)v & 0xFFFF800000000000UL) == 0xFFFF800000000000UL);
    return (dma_addr64)v & ~0xFFFF800000000000UL;
}

void*
kernel::phys_to_virt_maybe_0(dma_addr64 p)
{
    kassert((p & 0xFFFF800000000000UL) == 0);
    return (void*)(0xFFFF800000000000UL | p);
}

void*
kernel::phys_to_virt(dma_addr64 p)
{
    kassert(p != 0);
    return phys_to_virt_maybe_0(p);
}

dma_addr64
kernel::xlate(const void* v)
{
    return kernel_task->pt.xlate(v);
}
