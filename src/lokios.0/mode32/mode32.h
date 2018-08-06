#ifndef __MODE32_MODE32_H
#define __MODE32_MODE32_H

#include "hdr/kassert.h"
#include "hdr/compiler.h"
#include <stdint.h>

extern "C"
{
#define FLAG_BOOT_TYPE_MASK 0x00000001
#define FLAG_BOOT_TYPE_MBR  0x00000000
#define FLAG_BOOT_TYPE_PXE  0x00000001
    int m32_entry(uint32_t flags) __REG_PARMS__;
    void m32_smp_entry();
}

#endif /* __MODE32_MODE32_H */
