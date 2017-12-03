#ifndef __KERNEL_X86_H
#define __KERNEL_X86_H

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

#endif /* __KERNEL_X86_H */
