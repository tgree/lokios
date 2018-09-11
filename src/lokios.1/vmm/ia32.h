#ifndef __KERNEL_VMM_IA32_H
#define __KERNEL_VMM_IA32_H

#include <stdint.h>

namespace vmm
{
#define IA32_REG_8_16_32_64(r64,r32,r16,r8u,r8l) \
        union                \
        {                    \
            uint64_t r64;    \
            uint32_t r32;    \
            uint16_t r16;    \
            struct           \
            {                \
                uint8_t r8l; \
                uint8_t r8u; \
            };               \
        }
#define IA32_REG_16_32_64(r64,r32,r16) \
        union                \
        {                    \
            uint64_t r64;    \
            uint32_t r32;    \
            uint16_t r16;    \
        }
    struct ia32_gprs
    {
        union
        {
            struct
            {
                IA32_REG_8_16_32_64(rax,eax,ax,ah,al);
                IA32_REG_8_16_32_64(rbx,ebx,bx,bh,bl);
                IA32_REG_8_16_32_64(rcx,ecx,cx,ch,cl);
                IA32_REG_8_16_32_64(rdx,edx,dx,dh,dl);
                IA32_REG_16_32_64(rbp,ebp,bp);
                IA32_REG_16_32_64(rsi,esi,si);
                IA32_REG_16_32_64(rdi,edi,di);
                IA32_REG_16_32_64(rsp,esp,sp);
            };
            uint64_t r[16];
        };
    };

    struct ia32_sr
    {
        uint16_t    selector;
        uint32_t    base;
        uint32_t    limit;
    };

    struct ia32_srs
    {
        ia32_sr cs;
        ia32_sr ds;
        ia32_sr ss;
        ia32_sr es;
        ia32_sr fs;
        ia32_sr gs;
    };

    struct ia32_dtr
    {
        uint32_t    base;
        uint16_t    limit;
    };

    struct ia32_cpu
    {
        ia32_gprs   gprs;
        ia32_srs    srs;
        uint64_t    rflags;
        uint64_t    rip;
        uint64_t    cr[5];
        uint64_t    xcr[1];
        uint64_t    dr[8];
        ia32_dtr    idtr;
        ia32_dtr    gdtr;
        ia32_dtr    ldtr;
        ia32_dtr    tr;

        void hard_reset();
    };
}

#endif /* __KERNEL_VMM_IA32_H */
