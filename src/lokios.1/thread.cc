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

    // Set the thread's starting register values.  A note on stack alignment:
    // the ABI says that RSP must be 16-byte aligned *before* any call
    // instruction, meaning that after the call instruction it will be out of
    // alignment by 8 bytes (because the call instruction pushes the return
    // address onto the stack).  Since we are basically invoking a function
    // with this setup, and we want to ensure everything is properly aligned,
    // we need to ensure that RSP % 16 == 8.
    tcb.self        = &tcb;
    tcb.rflags      = 0;
    tcb.cs          = 8;
    tcb.ss          = 0;
    tcb.stack_guard = 0xA1B2C3D4E5F60718;
    tcb.rip         = (uint64_t)fn;
    tcb.rsp         = (uint64_t)stack + sizeof(stack) - 56;
    tcb.rax         = 0x4E4F4E4F4E4F4E4F;
    tcb.rbx         = 0x4E4F4E4F4E4F4E4F;
    tcb.rcx         = 0x4E4F4E4F4E4F4E4F;
    tcb.rdx         = 0x4E4F4E4F4E4F4E4F;
    tcb.rdi         = 0x4E4F4E4F4E4F4E4F;
    tcb.rsi         = 0x4E4F4E4F4E4F4E4F;
    tcb.rbp         = 0x4E4F4E4F4E4F4E4F;
    for (auto& r : tcb.r)
        r = 0x4E4F4E4F4E4F4E4F;
    kassert(tcb.rsp % 16 == 8);
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
