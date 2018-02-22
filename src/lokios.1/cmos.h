#ifndef __KERNEL_CMOS_H
#define __KERNEL_CMOS_H

#include "kassert.h"

namespace kernel
{
    struct date_time
    {
        union
        {
            struct
            {
                uint64_t    second : 8,
                            minute : 8,
                            hour   : 8,
                            day    : 8,
                            month  : 8,
                            year   : 24;
            };
            uint64_t    val;
        };

        constexpr date_time(uint32_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint8_t second):
            second(second),
            minute(minute),
            hour(hour),
            day(day),
            month(month),
            year(year)
        {
        }
        inline date_time() {}
    };
    constexpr bool operator==(const date_time& lhs, const date_time& rhs)
    {
        return lhs.val == rhs.val;
    }
    constexpr bool operator<(const date_time& lhs, const date_time& rhs)
    {
        return lhs.val < rhs.val;
    }
    KASSERT(sizeof(date_time) == 8);

    date_time read_cmos_date_time();

    void init_cmos();
}

#endif /* __KERNEL_CMOS_H */
