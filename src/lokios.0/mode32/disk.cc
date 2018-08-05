#include "disk.h"

int
m32_disk_read(uint8_t drive, uint32_t first_sector, uint16_t nsectors,
    void* dst)
{
    disk_read_io io;
    io.iorec_size        = sizeof(io);
    io.zero              = 0;
    io.nsectors          = nsectors;
    io.dst               = dst;
    io.first_sector_low  = first_sector;
    io.first_sector_high = 0;

    return _m32_disk_read(drive,&io);
}
