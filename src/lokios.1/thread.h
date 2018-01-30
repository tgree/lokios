#ifndef __KERNEL_THREAD_H
#define __KERNEL_THREAD_H

#include "mm/page_table.h"
#include "hdr/compiler.h"

namespace kernel
{
    struct task;

    typedef uint16_t thread_id;

    struct fxsave_area
    {
        uint16_t    fcw;
        uint16_t    fsw;
        uint8_t     ftw;
        uint8_t     rsrv1;
        uint16_t    fop;
        uint64_t    fip;
        uint64_t    fdp;
        uint32_t    mxcsr;
        uint32_t    mxcsr_mask;
        uint128_t   sromm[8];
        uint128_t   xmm[16];
        uint128_t   rsrv2[3];
        uint8_t     avail[48];  // 464 - Used as IRET frame for thread bouncing.
    };
    KASSERT(sizeof(fxsave_area) == 512);
    KASSERT(offsetof(fxsave_area,avail) == 464);

    struct tls_tcb
    {
        // Pointer to ourself.  This is used to find the address of the end of
        // TLS for expressions that take the address of a TLS variable.  This
        // is required because the FS register doesn't actually tell us the
        // address MSR_FS_BASE, so compilers such as gcc expect this "loopback"
        // pointer here, load it into a register and then subtract the TLS
        // offset to get the variable's address.
        tls_tcb*        self;           // 0

        // Our own fields.
        kdlink          link;           // 8

        // Register save/restore area.
        uint64_t        rflags;         // 24
        uint16_t        cs;             // 32
        uint16_t        ss;             // 34
        uint32_t        rsrv1;          // 36

        // Random value (initialized by the OS) for each thread that gcc uses
        // for stack-stomp checking.  It's required to be at FS:0x28.
        uint64_t        stack_guard;    // 40

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
        fxsave_area fxsa;               // 192

        // More free space that we could use for other stuff.
        uint64_t    rsrv2[168];         // 704
    };
    KASSERT(offsetof(tls_tcb, stack_guard) == 0x28);
    KASSERT(sizeof(tls_tcb) == 2048);
    KASSERT(offsetof(tls_tcb, fs_base) == 48);
    KASSERT(offsetof(tls_tcb, fxsa) % 16 == 0);
    KASSERT(offsetof(tls_tcb, fxsa) == 192);

    struct thread
    {
        uint8_t     guard1[4096];
        uint8_t     stack[16384];
        uint8_t     guard2[4096];
        uint8_t     tls[2048];
        tls_tcb     tcb;
        uint8_t     unmapped_but_free_if_we_need_it[4096];

        static  void*   operator new(size_t size);
        static  void    operator delete(void*);
                void    bounce(void (*fn)());

        thread(void (*entry_fn)(), bool enable_interrupts = true);
    };
    KASSERT(offsetof(thread,tcb) == 26624);
    KASSERT(offsetof(thread,tcb.fxsa.avail) == 27280);
    KASSERT(sizeof(thread) == 32768);

    static inline tls_tcb* get_current_tcb()
    {
        tls_tcb* tcb;
        asm ("mov %%fs:0, %0" : "=r"(tcb));
        return tcb;
    }

    inline thread_id get_thread_id()
    {
        return (thread_id)((uintptr_t)get_current_tcb() >> 16);
    }

    static inline thread* get_current_thread()
    {
        return container_of(get_current_tcb(),thread,tcb);
    }
}

#endif /* __KERNEL_THREAD_H */
