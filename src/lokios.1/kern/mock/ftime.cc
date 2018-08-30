#include "../time.h"

uint64_t fake_jiffies = 0;

uint64_t
kernel::get_jiffies()
{
    return fake_jiffies;
}
