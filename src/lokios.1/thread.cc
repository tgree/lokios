#include "thread.h"
#include "task.h"
#include "mm/mm.h"

extern uint8_t _tdata_begin[];
extern uint8_t _tdata_size[];
extern uint8_t _tbss_begin[];
extern uint8_t _tbss_end[];
extern uint8_t _tbss_size[];

kernel::thread::thread(void (*fn)())
{
    memset(stack,0,sizeof(stack));
    memset(tls,0,sizeof(tls));
    char* p = (char*)tls + sizeof(tls) - (uint64_t)_tbss_size -
              (uint64_t)_tdata_size;
    memcpy(p,_tdata_begin,(uint64_t)_tdata_size);

    tcb.self        = &tcb;
    tcb.stack_guard = 0xA1B2C3D4E5F60718;
    tcb.rip         = (uint64_t)fn;
    tcb.rdi         = 0x1111111111111111;
    tcb.rsp         = (uint64_t)stack + sizeof(stack) - 64;
}

void*
kernel::thread::operator new(size_t count, thread_id tid, task* task)
{
    kassert(count == sizeof(thread));

    thread* t = get_thread_region(tid);
    for (size_t i=0; i<sizeof(t->stack)/PAGE_SIZE; ++i)
    {
        task->pt.map_4k_page(t->stack + i*PAGE_SIZE,(uint64_t)page_alloc(),
                             PAGE_FLAG_WRITEABLE |
                             PAGE_FLAG_NOEXEC |
                             PAGE_CACHE_WB);
    }
    task->pt.map_4k_page(t->tls,(uint64_t)page_alloc(),
                         PAGE_FLAG_WRITEABLE |
                         PAGE_FLAG_NOEXEC |
                         PAGE_CACHE_WB);

    t->tcb.task = task;
    return t;
}

void
kernel::thread::operator delete(void* p, thread_id tid, task* task)
{
    // This is called if the thread constructor for thread id 0 throws an
    // exception.  We should free the physical pages and unmap them from the
    // page table.
    kassert(((uint64_t)p & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000);
    kassert(((uint64_t)p & 0x000000000000FFFF) == 0);
    kassert((((uint64_t)p >> 16) & 0xFFFF) == tid);
    kernel::panic();
}

void
kernel::thread::operator delete(void* p)
{
    // This is called for a regular delete.  Here, too, we should free the
    // physical pages and unmap them from the page table.  We can access the
    // page table via the pointer in the thread control block.
    kassert(((uint64_t)p & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000);
    kassert(((uint64_t)p & 0x000000000000FFFF) == 0);
    kernel::panic();
}
