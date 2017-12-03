#ifndef __KERNEL_TYPES_H
#define __KERNEL_TYPES_H

namespace kernel
{
    struct non_copyable
    {
        void operator=(const non_copyable&) = delete;

        non_copyable() = default;
        non_copyable(const non_copyable&) = delete;
    };
}

#endif /* __KERNEL_TYPES_H */
