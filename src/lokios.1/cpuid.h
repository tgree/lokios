#ifndef __KERNEL_CPUID_H
#define __KERNEL_CPUID_H

#include <stdint.h>

namespace kernel
{
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
}

#endif /* __KERNEL_CPUID_H */
