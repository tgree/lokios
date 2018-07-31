#ifndef __LOKIOS_TYPES_H
#define __LOKIOS_TYPES_H

namespace loki
{
    // Type that cannot be copied if you subclass it.
    struct non_copyable
    {
        void operator=(const non_copyable&) = delete;

        non_copyable() = default;
        non_copyable(const non_copyable&) = delete;
    };
}

#endif /* __LOKIOS_TYPES_H */
