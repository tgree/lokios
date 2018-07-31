#ifndef __LOKIOS_TYPES_H
#define __LOKIOS_TYPES_H

#include <stddef.h>

namespace loki
{
    // Given a pointer to a field in an object of some type, extract a pointer
    // to the enclosing object.
#define container_of(p,type,field) \
        ((type*)((char*)(1 ? (p) : &((type *)0)->field) - offsetof(type,field)))

    // Type that cannot be copied if you subclass it.
    struct non_copyable
    {
        void operator=(const non_copyable&) = delete;

        non_copyable() = default;
        non_copyable(const non_copyable&) = delete;
    };
}

#endif /* __LOKIOS_TYPES_H */
