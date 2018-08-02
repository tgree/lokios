#include "image.h"

extern char _kernel_virt_base[];
extern char _kernel_end[];

const kernel::kimage_header* kernel::image_header;
char* kernel::kernel_begin;
char* kernel::kernel_end;
char* kernel::elf_begin;
char* kernel::elf_end;

void
kernel::init_image()
{
    image_header = (kernel::kimage_header*)(uintptr_t)_kernel_virt_base;
    kernel_begin = (char*)image_header;
    kernel_end   = (char*)(uintptr_t)_kernel_end;
    elf_begin    = kernel_end;
    elf_end      = kernel_begin + image_header->num_sectors*512;
}
