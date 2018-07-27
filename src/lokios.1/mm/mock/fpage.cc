#include "../page.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>

size_t page_alloc_count;
size_t page_free_count;

static uint64_t paddr = 0x0000000010000000;

void*
kernel::page_alloc()
{
    void* p;
    p = mmap((void*)paddr,PAGE_SIZE,PROT_READ | PROT_WRITE,
             MAP_FIXED | MAP_ANON | MAP_PRIVATE,
             -1,0);
    kassert(p != MAP_FAILED);
    ++page_alloc_count;
    paddr += PAGE_SIZE;
    return p;
}

void
kernel::page_free(void* p)
{
    ++page_free_count;
    munmap(p,PAGE_SIZE);
}

size_t
kernel::page_count_free()
{
    return 0;
}
