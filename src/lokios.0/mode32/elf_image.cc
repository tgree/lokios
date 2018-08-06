#include "elf_image.h"
#include "console.h"
#include "kernel/image.h"
#include <string.h>

extern uint8_t _elf_base[];
extern uint8_t _kernel_base[];

int
process_elf_image(image_stream* is, elf_header* sector0)
{
    console::printf("Processing elf kernel image.\n");

    // Copy the first sector into place.
    auto* ehdr = (elf_header*)_elf_base;
    memcpy(ehdr,sector0,512);

    // Read sectors until we have the kernel header.
    uint16_t nsectors = (_kernel_base - _elf_base)/512;
    int err = is->read((char*)ehdr + 512,nsectors);
    if (err)
    {
        console::printf("Error %d reading up to kernel header sector.\n",err);
        return err;
    }

    // Sanity.
    auto* khdr = (kernel::image_header*)_kernel_base;
    if (khdr->sig != IMAGE_HEADER_SIG)
    {
        console::printf("ELF file was not a lokios kernel.\n");
        return -1;
    }

    // Read the remaining sectors.
    err = is->read((char*)khdr + 512,khdr->num_sectors-1);
    if (err)
    {
        console::printf("Error %d reading remaining sectors.\n",err);
        return err;
    }

    return 0;
}
