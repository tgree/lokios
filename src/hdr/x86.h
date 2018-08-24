#ifndef __LOKIOS_X86_H
#define __LOKIOS_X86_H

#include "compiler.h"
#include "kassert.h"
#include <stdint.h>

static inline uint64_t mfcr0()
{
    uint64_t rc;
    asm ("mov %%cr0, %0" : "=r"(rc));
    return rc;
}

static inline uint64_t mfcr2()
{
    uint64_t rc;
    asm ("mov %%cr2, %0" : "=r"(rc));
    return rc;
}

static inline uint64_t mfcr3()
{
    uint64_t rc;
    asm ("mov %%cr3, %0" : "=r"(rc));
    return rc;
}

static inline uint64_t mfcr4()
{
    uint64_t rc;
    asm ("mov %%cr4, %0" : "=r"(rc));
    return rc;
}

static inline uint64_t mfcr8()
{
    uint64_t rc;
    asm ("mov %%cr8, %0" : "=r"(rc));
    return rc;
}

static inline void mtcr3(uint64_t val)
{
    asm ("mov %0, %%cr3" : : "r"(val));
}

static inline void mtcr4(uint64_t val)
{
    asm ("mov %0, %%cr4" : : "r"(val));
}

static inline void lgdt(uint64_t base, uint16_t limit)
{
    struct __PACKED__
    {
        uint16_t    limit;
        uint64_t    base;
    } gdtpd = {limit,base};
    KASSERT(sizeof(gdtpd) == 10);
    asm ("lgdt %0" : : "m"(gdtpd));
}

static inline void lidt(uint64_t base, uint16_t limit)
{
    struct __PACKED__
    {
        uint16_t    limit;
        uint64_t    base;
    } idtpd = {limit,base};
    KASSERT(sizeof(idtpd) == 10);
    asm ("lidt %0" : : "m"(idtpd));
}

static inline void ltr(uint16_t val)
{
    asm ("ltr %0" : : "r"(val));
}

static inline void int126()
{
    asm ("int $126");
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t rc;
    asm volatile ("inb %1" : "=a"(rc) : "d"(port));
    return rc;
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t rc;
    asm volatile ("inw %1" : "=a"(rc) : "d"(port));
    return rc;
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t rc;
    asm volatile ("inl %1" : "=a"(rc) : "d"(port));
    return rc;
}

static inline void outb(uint8_t val, uint16_t port)
{
    asm ("outb %0, %1" : : "a"(val), "d"(port));
}

static inline void outw(uint16_t val, uint16_t port)
{
    asm ("outw %0, %1" : : "a"(val), "d"(port));
}

static inline void outl(uint32_t val, uint16_t port)
{
    asm ("outl %0, %1" : : "a"(val), "d"(port));
}

static inline void cpu_enable_interrupts()
{
    asm ("sti");
}

static inline void cpu_disable_interrupts()
{
    asm ("cli");
}

static inline void cpu_pause()
{
    asm ("pause");
}

static inline void cpu_halt()
{
    asm ("hlt");
}

static inline uint64_t get_rflags()
{
    uint64_t rc;
    asm ("pushfq; pop %0;" : "=r"(rc));
    return rc;
}

static inline uint64_t rdtsc()
{
    uint64_t lsw;
    uint64_t msw;
    asm volatile("rdtsc" : "=a"(lsw), "=d"(msw));
    return (msw << 32) | lsw;
}

#define IA32_APIC_BASE          0x0000001B
#define IA32_EFER               0xC0000080
#define IA32_FS_BASE            0xC0000100
#define IA32_GS_BASE            0xC0000101
#define IA32_KERNEL_GS_BASE     0xC0000102

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t eax;
    uint32_t edx;
    asm ("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr));
    return ((uint64_t)edx << 32) | (uint64_t)eax;
}

static inline void wrmsr(uint64_t val, uint32_t msr)
{
    uint32_t eax = val;
    uint32_t edx = (val >> 32);
    asm ("wrmsr" : : "a"(eax), "c"(msr), "d"(edx));
}

struct cpuid_result
{
    uint32_t    eax;
    uint32_t    ebx;
    uint32_t    ecx;
    uint32_t    edx;
};

inline void cpuid(uint32_t eax, uint32_t ecx, uint32_t* reax,
                  uint32_t* rebx, uint32_t* recx, uint32_t* redx)
{
    asm("cpuid"
        : "=a"(*reax), "=b"(*rebx), "=c"(*recx), "=d"(*redx)
        : "a"(eax), "c"(ecx));
}

inline void cpuid(uint32_t eax, uint32_t ecx, void* r)
{
    return cpuid(eax,ecx,(uint32_t*)r+0,(uint32_t*)r+1,(uint32_t*)r+2,
                 (uint32_t*)r+3);
}

inline cpuid_result cpuid(uint32_t eax, uint32_t ecx = 0)
{
    cpuid_result r;
    cpuid(eax,ecx,&r.eax,&r.ebx,&r.ecx,&r.edx);
    return r;
}

struct tss64
{
    uint32_t    rsrv0;
    uint64_t    rsp[3];
    uint64_t    ist[8];     // IST0 is reserved.
    uint8_t     rsrv1[10];
    uint16_t    iomap_base;
} __PACKED__;
KASSERT(sizeof(tss64) == 104);

struct no_iomap_tss64
{
    tss64       tss;
    uint16_t    ones;
} __PACKED__;
KASSERT(sizeof(no_iomap_tss64) == 106);

#define TSS_DESC_0(base,limit) \
    ((((base)  << 32) & 0xFF00000000000000) | \
     (((limit) << 32) & 0x000F000000000000) | \
                        0x0000890000000000  | \
     (((base)  << 16) & 0x000000FFFFFF0000) | \
     (((limit) <<  0) & 0x000000000000FFFF))
#define TSS_DESC_1(base,limit) \
    (((base) >> 32) & 0x00000000FFFFFFFF)

#endif /* __LOKIOS_X86_H */
