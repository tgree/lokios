#ifndef __KERNEL_IMAGE_H
#define __KERNEL_IMAGE_H

namespace kernel
{
#define IMAGE_HEADER_SIG    0x494B4F4C // 'LOKI'
    struct image_header
    {
        uint32_t    sig;
        uint32_t    num_sectors;
        uint32_t    page_table_addr;
    };

#define IMAGE_FOOTER_SIG    0x4C4F4B49 // 'IKOL'
    struct image_footer
    {
        uint8_t     dd[508];
        uint32_t    sig;
    };
}

#endif /* __KERNEL_IMAGE_H */
