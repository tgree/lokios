#ifndef __KERNEL_PLATFORM_QEMU_H
#define __KERNEL_PLATFORM_QEMU_H

#include "platform.h"

#define QEMU_BRAND_SIGNATURE char_code("QEMU")

namespace kernel
{
    struct qemu_platform : public platform
    {
        virtual void _exit_guest(int status);
        virtual void _reboot();

        qemu_platform();
    };
}

#endif /* __KERNEL_PLATFORM_QEMU_H */
