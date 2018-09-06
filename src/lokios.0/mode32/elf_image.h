#ifndef __MODE32_ELF_IMAGE_H
#define __MODE32_ELF_IMAGE_H

#include "mode32.h"

#define ELF_HEADER_SIG char_code("\x7F""ELF")

int process_elf_image(image_stream* is, void* sector0, uintptr_t* image_end);

#endif /* __MODE32_ELF_IMAGE_H */
