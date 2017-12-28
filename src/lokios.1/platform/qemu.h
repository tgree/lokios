#ifndef __KERNEL_PLATFORM_QEMU_H
#define __KERNEL_PLATFORM_QEMU_H

#include "platform.h"

#define QEMU_BRAND_SIGNATURE    0x554D4551

namespace kernel
{
    struct qemu_platform : public platform
    {
        virtual void _exit_guest(int status);

        qemu_platform();
    };
}

#endif /* __KERNEL_PLATFORM_QEMU_H */
