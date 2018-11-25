#ifndef __KERNEL_VMM_IMAGE_H
#define __KERNEL_VMM_IMAGE_H

#include "k++/klist.h"
#include "wapi/wapi.h"

namespace vmm
{
    struct name_too_long_exception {};

    struct image
    {
        kernel::kdlink      link;
        wapi::node          wapi_node;
        const void* const   addr;
        const size_t        len;
        uint8_t             md5[16];

        // Addr/len must be buddy allocator parameters; we are taking ownership
        // of the buddy block which will get freed when the image is destroyed.
        image(const char* name, const void* addr, size_t len);
        ~image();
    };

    extern kernel::kdlist_leaks<vmm::image> images;
}

#endif /* __KERNEL_VMM_IMAGE_H */
