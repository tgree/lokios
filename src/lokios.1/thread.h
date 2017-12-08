#ifndef __KERNEL_THREAD_H
#define __KERNEL_THREAD_H

#include "mm/page_table.h"

namespace kernel
{
    struct task;

    typedef uint16_t thread_id;

    struct tls_tcb
    {
        // Pointer to ourself.  This is used to find the address of the end of
        // TLS for expressions that take the address of a TLS variable.  This
        // is required because the FS register doesn't actually tell us the
        // address MSR_FS_BASE, so compilers such as gcc expect this "loopback"
        // pointer here, load it into a register and then subtract the TLS
        // offset to get the variable's address.
        tls_tcb*        self;

        // Our own fields.
        klink           link;
        kernel::task*   task;
        uint64_t        rsrv[2];

        // Random value (initialized by the OS) for each thread that gcc uses
        // for stack-stomp checking.  It's required to be at FS:0x28.
        uint64_t        stack_guard;

        // Register save/restore area.
        uint64_t    fs_base;            // 48
        uint64_t    rip;                // 56
        uint64_t    rsp;                // 64
        uint64_t    rax;                // 72
        uint64_t    rbx;                // 80
        uint64_t    rcx;                // 88
        uint64_t    rdx;                // 96
        uint64_t    rdi;                // 104
        uint64_t    rsi;                // 112
        uint64_t    rbp;                // 120
        uint64_t    r[8];               // 128 + 8*i

        // More free space that we could use for other stuff.
        uint64_t    rsrv2[232];
    };
    KASSERT(offsetof(tls_tcb, stack_guard) == 0x28);
    KASSERT(sizeof(tls_tcb) == 2048);
    KASSERT(offsetof(tls_tcb, fs_base) == 48);

    struct thread
    {
        uint8_t     guard1[4096];
        uint8_t     stack[16384];
        uint8_t     guard2[4096];
        uint8_t     tls[2048];
        tls_tcb     tcb;
        uint8_t     unmapped_but_free_if_we_need_it[4096];

        static void* operator new(size_t count, thread_id tid, task* t);
        static void operator delete(void* p, thread_id tid, task* t);
        static void operator delete(void*);

        thread(void (*entry_fn)());
    };
    KASSERT(sizeof(thread) == 32768);
}

#endif /* __KERNEL_THREAD_H */
