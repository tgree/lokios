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

static inline void int32()
{
    asm ("int $32");
}

#endif /* __KERNEL_X86_H */
