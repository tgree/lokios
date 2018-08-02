#ifndef __KERNEL_IMAGE_H
#define __KERNEL_IMAGE_H

#include <stdint.h>

namespace kernel
{
#define LOKI_IMAGE_MAGIC 0x494B4F4C
    struct kimage_header
    {
        uint32_t    magic;
        uint32_t    num_sectors;
    };

    extern const kimage_header* image_header;
    extern char* kernel_begin;
    extern char* kernel_end;
    extern char* elf_begin;
    extern char* elf_end;

    void init_image();
}

#endif /* __KERNEL_IMAGE_H */
