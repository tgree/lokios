#include "thread.h"
#include "task.h"
#include "mm/mm.h"
#include <new>

extern uint8_t _tdata_begin[];
extern uint8_t _tdata_size[];
extern uint8_t _tbss_begin[];
extern uint8_t _tbss_end[];
extern uint8_t _tbss_size[];

struct thread_mem
{
    uint8_t         guard1[4096];
    uint8_t         stack[16384];
    uint8_t         guard2[4096];
    uint8_t         tls[2048];
    kernel::klink   link;
};

static kernel::spinlock             free_threads_lock;
static kernel::klist<thread_mem>    free_threads;
static uint16_t                     next_free_tid = 1;

typedef void (*bounce_fn)(kernel::thread* t, void (*fn)());

kernel::thread::thread(void (*fn)(), bool enable_interrupts)
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
    tcb.rflags      = 0x2 | (enable_interrupts ? 0x200 : 0);
    tcb.cs          = 8;
    tcb.ss          = 0;
    tcb.stack_guard = 0xA1B2C3D4E5F60718;
    tcb.rip         = (uint64_t)(bounce_fn)&kernel::thread::bounce;
    tcb.rsp         = (uint64_t)stack + sizeof(stack) - 56;
    tcb.rax         = 0x4E4F4E4F4E4F4E4F;
    tcb.rbx         = 0x4E4F4E4F4E4F4E4F;
    tcb.rcx         = 0x4E4F4E4F4E4F4E4F;
    tcb.rdx         = 0x4E4F4E4F4E4F4E4F;
    tcb.rdi         = (uint64_t)this;
    tcb.rsi         = (uint64_t)fn;
    tcb.rbp         = 0x4E4F4E4F4E4F4E4F;
    for (auto& r : tcb.r)
        r = 0x4E4F4E4F4E4F4E4F;
    kassert(tcb.rsp % 16 == 8);

    memset(&tcb.fxsa,0,sizeof(tcb.fxsa));
}

void
kernel::thread::bounce(void (*fn)())
{
    (*fn)();
}

void*
kernel::thread::operator new(size_t size)
{
    kassert(size == sizeof(thread));

    // Try recycling a freed thread.
    uint16_t tid;
    with (free_threads_lock)
    {
        if (!free_threads.empty())
        {
            thread_mem* tm = klist_front(free_threads,link);
            free_threads.pop_front();
            tm->~thread_mem();
            return tm;
        }
        kassert(next_free_tid != 0);
        tid = next_free_tid++;
    }

    // Allocate a new thread.
    thread_mem* tm = (thread_mem*)get_thread_region(tid);
    for (size_t i=0; i<sizeof(tm->stack)/PAGE_SIZE; ++i)
    {
        kernel_task->pt.map_4k_page(tm->stack + i*PAGE_SIZE,
                                    virt_to_phys(page_alloc()),
                                    PAGE_FLAGS_DATA);
    }
    kernel_task->pt.map_4k_page(tm->tls,virt_to_phys(page_alloc()),
                                PAGE_FLAGS_DATA);
    return tm;
}

void
kernel::thread::operator delete(void* p)
{
    kassert(((uint64_t)p & 0xFFFFFFFF0000FFFF) == 0xFFFFFFFF00000000);
    thread_mem* tm = new(p) thread_mem;
    with (free_threads_lock)
    {
        free_threads.push_back(&tm->link);
    }
}
