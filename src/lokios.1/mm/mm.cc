#include "mm.h"
#include "page_table.h"
#include "sbrk.h"
#include "../x86.h"
#include "../console.h"
#include "../task.h"

void
kernel::init_mm(const e820_map* m)
{
    // Walk the page tables to find the last mapped address.
    uintptr_t top_addr = 0;
    for (const auto pte : page_table_leaf_iterator((uint64_t*)mfcr3()))
        top_addr = pte.get_paddr() + pte.get_len();
    vga->printf("End of mapped RAM: 0x%016lX\n",top_addr);

    // Pre-initialize the page list.
    page_preinit(m,top_addr);
    
    // Print out the page stats.
    size_t free_pages = page_count_free();
    vga->printf("Free mapped RAM: %zuMB (%zu 4K pages)\n",
                free_pages/256,free_pages);

    // Print out the sbrk stats.
    vga->printf("  Free sbrk RAM: %luK\n",
                ((uint64_t)get_sbrk_limit() - (uint64_t)sbrk(0))/1024);
}

void*
kernel::pmap(uint64_t paddr, uint64_t flags)
{
    uint64_t paddr_2m = round_down_pow2(paddr,0x00200000);
    uint64_t vaddr_2m = 0xFFFF800000000000UL | paddr_2m;
    get_current_tcb()->task->pt.map_2m_page((void*)vaddr_2m,paddr_2m,flags);
    return (void*)(vaddr_2m | (paddr & 0x001FFFFF));
}

void*
kernel::pmap_range(uint64_t paddr, size_t len, uint64_t flags)
{
    uint64_t start = round_down_pow2(paddr,0x00200000);
    uint64_t end   = round_up_pow2(paddr + len,0x00200000);
    void* vaddr    = pmap(paddr);
    for (uint64_t p = start + 0x00200000; p != end; p += 0x00200000)
        pmap(p,flags);
    return vaddr;
}