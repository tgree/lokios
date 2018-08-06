#include "raw_image.h"
#include "console.h"
#include <string.h>

extern uint8_t _kernel_base[];

int
process_raw_image(image_stream* is, kernel::image_header* sector0)
{
    console::printf("Processing raw kernel image.\n");

    // Copy the first sector into place.
    auto* khdr = (kernel::image_header*)_kernel_base;
    memcpy(khdr,sector0,512);

    // Read the remaining sectors.
    int err = is->read((char*)khdr + 512,khdr->num_sectors-1);
    if (err)
    {
        console::printf("Error %d reading remaining sectors.\n",err);
        return err;
    }

    return 0;
}
