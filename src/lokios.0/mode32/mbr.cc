#include "mbr.h"
#include "disk.h"
#include "console.h"
#include "kernel/image.h"
#include <stddef.h>

extern uint8_t _kernel_base[];
extern uint8_t _extra_sectors[];
extern uint8_t _mbr_drive_number;
extern uint8_t _pre_e820_bounce_buffer[];

void*
memcpy(void* _dest, const void* _src, size_t n)
{
    auto* d = (char*)_dest;
    auto* s = (const char*)_src;
    while (n--)
        *d++ = *s++;
    return _dest;
}

int
mbr_entry()
{
    uint32_t extra_sectors       = (uint32_t)_extra_sectors;
    uint32_t first_kernel_sector = extra_sectors + 1;
    uint8_t mbr_drive_number     = _mbr_drive_number;
    uint32_t* buf                = (uint32_t*)(uint32_t)_pre_e820_bounce_buffer;
    int err;

    err = disk_read(mbr_drive_number,first_kernel_sector,1,buf);
    if (err)
        return err;

    auto* khdr           = (kernel::image_header*)buf;
    uint32_t rem_sectors = khdr->num_sectors;
    uint32_t sector_num  = first_kernel_sector;
    auto* pos            = (char*)_kernel_base;
    while (rem_sectors)
    {
        uint32_t nsectors = (rem_sectors >= 8 ? 8 : rem_sectors);
        err = disk_read(mbr_drive_number,sector_num,nsectors,buf);
        if (err)
            return err;

        memcpy(pos,buf,512*nsectors);
        rem_sectors -= nsectors;
        sector_num  += nsectors;
        pos         += 512*nsectors;
    }

    console::printf("MBR entry complete.\n");
    return 0;
}
