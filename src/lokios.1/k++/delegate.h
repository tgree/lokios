#ifndef __KERNEL_DELEGATE_H
#define __KERNEL_DELEGATE_H

#include <utility>

namespace kernel
{
    template<typename Signature>
    struct delegate;

    template<typename RC, typename ...Args>
    struct delegate<RC(Args...)>
    {
        void*   obj;
        RC      (*bouncer)(void* obj, Args...);

        inline RC operator()(Args&&... args)
        {
            return (*bouncer)(obj,std::forward<Args>(args)...);
        }

        template<typename T, RC(T::*Method)(Args...)>
        static RC mbounce(void* obj, Args... args)
        {
            return (((T*)obj)->*Method)(std::forward<Args>(args)...);
        }

        static RC fbounce(void* func, Args... args)
        {
            return ((RC(*)(Args...))func)(std::forward<Args>(args)...);
        }
    };

    template<typename T, typename RC, typename ...Args>
    delegate<RC(Args...)> delegate_convert(RC (T::*Method)(Args...));

    template<typename RC, typename ...Args>
    delegate<RC(Args...)> delegate_convert(RC (*Func)(Args...));

#define method_delegate_ptmf(ptmf) \
    {(void*)this, \
     decltype(kernel::delegate_convert(ptmf))::mbounce<typeof(*this),ptmf>}

#define method_delegate(m) \
    method_delegate_ptmf(&std::remove_reference_t<decltype(*this)>::m)

#define func_delegate(f) delegate<typeof(f)> \
    {(void*)f, \
     decltype(kernel::delegate_convert(f))::fbounce}
}

#endif /* __KERNEL_DELEGATE_H */
