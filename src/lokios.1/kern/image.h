#ifndef __KERNEL_IMAGE_H
#define __KERNEL_IMAGE_H

#include "hdr/endian.h"

namespace kernel
{
#define IMAGE_HEADER_SIG char_code("LOKI")
    struct image_header
    {
        uint32_t    sig;
        uint32_t    num_sectors;
        uint32_t    page_table_addr;
    };

#define IMAGE_FOOTER_SIG char_code("IKOL")
    struct image_footer
    {
        uint8_t     dd[508];
        uint32_t    sig;
    };
}

#endif /* __KERNEL_IMAGE_H */
