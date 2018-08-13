#include "../random.h"

static uint32_t seed = 0x12345678;

uint32_t
kernel::random(uint32_t first, uint32_t last)
{
    // Not random at all... by design.
    uint64_t width = (uint64_t)last + 1UL - (uint64_t)first;
    return first + (seed++ % width);
}
