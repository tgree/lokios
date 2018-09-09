#include "mm.h"
#include "page_table.h"
#include "kern/console.h"
#include "kern/task.h"
#include "kern/cpu.h"
#include "kern/tlb.h"
#include "wapi/wapi.h"

using kernel::console::printf;

dma_addr64 kernel::zero_page_dma;
dma_addr64 kernel::trash_page_dma;
static uintptr_t top_addr;
static const kernel::e820_map* e820_map;

void
kernel::preinit_mm(const e820_map* m, dma_addr64 bitmap_base)
{
    // Walk the page tables to find the last mapped address.
    for (const auto pte : page_table_leaf_iterator(mfcr3()))
        top_addr = pte.get_paddr() + pte.get_len();
    printf("End of bootloader-mapped RAM: 0x%016lX\n",top_addr);

    // Pre-initialize the page list.
    page_preinit(m,top_addr,bitmap_base);
    
    // Set up the zero/trash pages and save the e820 map for wapi.
    zero_page_dma  = virt_to_phys(page_zalloc());
    trash_page_dma = virt_to_phys(page_zalloc());
    ::e820_map     = m;
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
    void* vaddr_4k    = phys_to_virt_maybe_0(paddr_4k);
    kernel_task->pt.map_4k_page(vaddr_4k,paddr_4k,flags);
    return phys_to_virt_maybe_0(paddr);
}

void*
kernel::pmap_range(dma_addr64 paddr, size_t len, uint64_t flags)
{
    uint64_t start = round_down_pow2(paddr,PAGE_SIZE);
    uint64_t end   = round_up_pow2(paddr + len,PAGE_SIZE);
    mmap(phys_to_virt(start),start,end-start,flags);
    return phys_to_virt(paddr);
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
    kassert(((uintptr_t)v & 0xFFFFF00000000000UL) == 0xFFFF800000000000UL ||
            ((uintptr_t)v & 0xFFFFFFFFFF000000UL) == 0xFFFFFFFFC0000000UL);
    if (((uintptr_t)v & 0xFFFFF00000000000UL) == 0xFFFF800000000000UL)
        return (dma_addr64)v & ~0xFFFF800000000000UL;
    return (dma_addr64)v & ~0xFFFFFFFFFF000000UL;
}

void*
kernel::phys_to_virt_maybe_0(dma_addr64 p)
{
    kassert((p & 0xFFFFF00000000000UL) == 0);
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

void
mm_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // GET /mm
    rsp->printf("{\r\n"
                "    \"bootloader_top_addr\" : \"0x%08lX\",\r\n"
                "    \"total_pages\"         : %zu,\r\n"
                "    \"free_pages\"          : %zu,\r\n"
                "    \"page_table_pages\"    : %zu\r\n"
                "}\r\n",
                top_addr,kernel::page_count_total(),kernel::page_count_free(),
                kernel::kernel_task->pt.page_count);
}

void
e820_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // Get /mm/e820
    char buf[22];
    rsp->printf("{\r\n"
                "    \"entries\" : [ ");
    for (size_t i=0; i<::e820_map->nentries; ++i)
    {
        auto* e = &::e820_map->entries[i];
        rsp->printf("\r\n        { \"base\" : \"0x%016lX\", "
                    "\"len\" : \"0x%016lX\", \"attrs\" : \"0x%08X\", "
                    "\"type\" : \"%s\" },",
                    e->base,e->len,e->extended_attrs,
                    kernel::get_e820_type_string(e->type,buf));
    }
    rsp->ks.shrink();
    rsp->printf("\r\n        ]\r\n}\r\n");
}

static wapi::global_node mm_wapi_node(&wapi::root_node,
                                       func_delegate(mm_request),
                                       METHOD_GET_MASK,"mm");
static wapi::node e820_wapi_node(&mm_wapi_node,func_delegate(e820_request),
                                 METHOD_GET_MASK,"e820");
