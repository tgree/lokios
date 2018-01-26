#ifndef __KERNEL_MSR_H
#define __KERNEL_MSR_H

#include <stdint.h>

#define IA32_APIC_BASE          0x0000001B
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

#endif /* __KERNEL_MSR_H */
