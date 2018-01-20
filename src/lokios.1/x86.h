#ifndef __KERNEL_X86_H
#define __KERNEL_X86_H

#include "hdr/compiler.h"
#include "kernel/kassert.h"

static inline uint64_t mfcr3()
{
    uint64_t rc;
    asm ("mov %%cr3, %0" : "=r"(rc));
    return rc;
}

static inline void mtcr3(uint64_t val)
{
    asm ("mov %0, %%cr3" : : "r"(val));
}

static inline uint64_t mfcr4()
{
    uint64_t rc;
    asm ("mov %%cr4, %0" : "=r"(rc));
    return rc;
}

static inline void mtcr4(uint64_t val)
{
    asm ("mov %0, %%cr4" : : "r"(val));
}

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t eax;
    uint32_t edx;
    asm ("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr));
    return ((uint64_t)edx << 32) | (uint64_t)eax;
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
    asm volatile ("inb %1" : "=a"(rc) : "di"(port));
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

#endif /* __KERNEL_X86_H */
