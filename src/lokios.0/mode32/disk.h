#ifndef __MODE32_DISK_H
#define __MODE32_DISK_H

#include "hdr/compiler.h"
#include "hdr/kassert.h"
#include <stdint.h>

struct __PACKED__ disk_read_io
{
    uint8_t     iorec_size;
    uint8_t     zero;
    uint16_t    nsectors;
    void*       dst;
    uint32_t    first_sector_low;
    uint32_t    first_sector_high;
};
KASSERT(sizeof(disk_read_io) == 16);

extern "C"
{
    int _m32_disk_read(uint8_t drive, disk_read_io* iopb);
    int m32_disk_read(uint8_t drive, uint32_t first_sector, uint16_t nsectors,
                      void* dst);
}

#endif /* __MODE32_DISK_H */
