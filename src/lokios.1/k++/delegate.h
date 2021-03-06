#ifndef __KERNEL_DELEGATE_H
#define __KERNEL_DELEGATE_H

#include "hdr/types.h"

namespace kernel
{
    template<typename Signature>
    struct delegate;

    template<typename RC, typename ...Args>
    struct delegate<RC(Args...)>
    {
        void*   obj;
        RC      (*bouncer)(void* obj, Args...);

        constexpr operator bool() const {return bouncer != NULL;}
        void clear()                    {bouncer = NULL;}

        inline RC operator()(Args... args)
        {
            return (*bouncer)(obj,loki::forward<Args>(args)...);
        }

        template<typename T, RC(T::*Method)(Args...)>
        static RC mbounce(void* obj, Args... args)
        {
            return (((T*)obj)->*Method)(loki::forward<Args>(args)...);
        }

        template<RC (&Func)(Args...)>
        static RC fbounce(void* junk, Args... args)
        {
            return Func(loki::forward<Args>(args)...);
        }

        template<typename T, RC(T::*Method)(Args...)>
        void _make_method_delegate(T* o)
        {
            obj     = (void*)o;
            bouncer = mbounce<T,Method>;
        }

        constexpr delegate(void* obj = NULL, typeof(bouncer) bouncer = NULL):
            obj(obj),bouncer(bouncer) {}
    };

    template<typename T, typename RC, typename ...Args>
    delegate<RC(Args...)> delegate_convert(RC (T::*Method)(Args...));

    template<typename RC, typename ...Args>
    delegate<RC(Args...)> delegate_convert(RC (*Func)(Args...));

#define make_method_delegate(m) \
    _make_method_delegate<this_type,&this_type::m>(this)

#define method_delegate_ptmf(ptmf) \
    decltype(kernel::delegate_convert(ptmf))((void*)this, \
     decltype(kernel::delegate_convert(ptmf))::mbounce<this_type,ptmf>)

#define method_delegate(m) \
    method_delegate_ptmf(&this_type::m)

#define func_delegate(f) kernel::delegate<typeof(f)> \
    (NULL, \
     decltype(kernel::delegate_convert(f))::fbounce<f>)
}

#endif /* __KERNEL_DELEGATE_H */
