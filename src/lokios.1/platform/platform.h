#ifndef __KERNEL_PLATFORM_PLATFORM_H
#define __KERNEL_PLATFORM_PLATFORM_H

#include "hdr/compiler.h"

namespace kernel
{
    struct platform
    {
        const char* const name;

        virtual void _exit_guest(int status) = 0;

        platform(const char* name);
    };

    void exit_guest(int status) __NORETURN__;

    void init_platform();
}

#endif /* __KERNEL_PLATFORM_PLATFORM_H */
