#include "mbr.h"
#include "disk.h"
#include "console.h"
#include "kernel/image.h"
#include <string.h>
#include <stddef.h>

extern uint8_t _kernel_base[];
extern uint8_t _extra_sectors[];
extern uint8_t _mbr_drive_number;
extern uint8_t _pre_e820_bounce_buffer[];

static int
mbr_read_sectors(void* dst, uint16_t nsectors)
{
    // Global state.
    static uint32_t sector_num = (uint32_t)_extra_sectors + 1;

    // Read all the sectors.
    uint32_t rem_sectors = nsectors;
    auto* pos            = (char*)dst;
    uint32_t* buf        = (uint32_t*)(uint32_t)_pre_e820_bounce_buffer;
    while (rem_sectors)
    {
        uint32_t nsectors = (rem_sectors >= 8 ? 8 : rem_sectors);
        int err = disk_read(_mbr_drive_number,sector_num,nsectors,buf);
        if (err)
            return err;

        memcpy(pos,buf,512*nsectors);
        rem_sectors -= nsectors;
        sector_num  += nsectors;
        pos         += 512*nsectors;
    }

    return 0;
}

int
mbr_entry()
{
    // Read the first sector into kernel_base.
    auto* khdr = (kernel::image_header*)(uint32_t)_kernel_base;
    int err = mbr_read_sectors(khdr,1);
    if (err)
        return err;

    // Validate the image header.
    if (khdr->sig != IMAGE_HEADER_SIG)
    {
        console::printf("Invalid kernel header signature.\n");
        return -6;
    }

    // Read the remaining sectors.
    err = mbr_read_sectors((char*)khdr + 512,khdr->num_sectors-1);
    if (err)
        return err;

    console::printf("MBR entry complete.\n");
    return 0;
}
