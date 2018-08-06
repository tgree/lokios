#ifndef __MODE32_ELF_IMAGE_H
#define __MODE32_ELF_IMAGE_H

#include "mode32.h"

#define ELF_HEADER_SIG 0x464C457F
struct elf_header
{
    uint32_t    sig;
};

int process_elf_image(image_stream* is, elf_header* sector0);

#endif /* __MODE32_ELF_IMAGE_H */
