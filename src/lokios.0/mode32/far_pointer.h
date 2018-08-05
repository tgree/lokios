#ifndef __MODE32_FAR_POINTER_H
#define __MODE32_FAR_POINTER_H

#include "hdr/kassert.h"
#include <stdint.h>

struct far_pointer
{
    uint16_t    offset;
    uint16_t    segment;

    inline void* to_addr()
    {
        return (void*)(((uint32_t)segment << 4) + (uint32_t)offset);
    }
};
KASSERT(sizeof(far_pointer) == 4);


#endif /* __MODE32_FAR_POINTER_H */
