#ifndef __KERNEL_IMAGE_H
#define __KERNEL_IMAGE_H

namespace kernel
{
    struct image_header
    {
        uint32_t    num_sectors;
        uint32_t    page_table_addr;
    };
}

#endif /* __KERNEL_IMAGE_H */
