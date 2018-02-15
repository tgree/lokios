#ifndef __KERNEL_FUNCTIONAL_H
#define __KERNEL_FUNCTIONAL_H

namespace kernel
{
    template<typename T>
    struct less
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs < rhs;
        }
    };

    template<typename T>
    struct greater
    {
        constexpr bool operator()(const T& lhs, const T& rhs) const
        {
            return lhs > rhs;
        }
    };
}

#endif /* __KERNEL_FUNCTIONAL_H */
