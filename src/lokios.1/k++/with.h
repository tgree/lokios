#ifndef __KERNEL_WITH_H
#define __KERNEL_WITH_H

namespace kernel
{
    template<typename T>
    struct with_raii_helper
    {
        T& t;
        inline with_raii_helper(T& t):t(t) {t.acquire();}
        inline ~with_raii_helper()         {t.release();}
    };
}

#define with(obj) \
    if constexpr( \
        kernel::with_raii_helper<decltype(obj)> __raii_with_helper__(obj); \
        true)

#endif /* __KERNEL_WITH_H */
