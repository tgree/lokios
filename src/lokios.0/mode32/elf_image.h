#ifndef __MODE32_ELF_IMAGE_H
#define __MODE32_ELF_IMAGE_H

#include "mode32.h"

#define ELF_HEADER_SIG 0x464C457F

int process_elf_image(image_stream* is, void* sector0);

#endif /* __MODE32_ELF_IMAGE_H */
