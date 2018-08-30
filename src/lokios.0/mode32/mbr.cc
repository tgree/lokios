#include "mbr.h"
#include "disk.h"
#include "kern/image.h"
#include <string.h>

extern uint8_t _kernel_base[];
extern uint8_t _extra_sectors[];
extern uint8_t _mbr_drive_number;
extern uint8_t _pre_e820_bounce_buffer[];

struct mbr_image_stream : public image_stream
{
    uint32_t sector_num;

    virtual int open() override
    {
        sector_num = (uint32_t)_extra_sectors + 1;
        return 0;
    }

    virtual int read(void* dst, uint16_t nsectors) override
    {
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

    virtual int close() override
    {
        return 0;
    }

    constexpr mbr_image_stream():
        image_stream("MBR"),
        sector_num(0)
    {
    }
};

static mbr_image_stream mbr_stream;

image_stream*
mbr_entry()
{
    return &mbr_stream;
}
