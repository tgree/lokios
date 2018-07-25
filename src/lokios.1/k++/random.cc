#include "random.h"
#include "kmath.h"

#define POLYNOMIAL 0x000000000000001BUL
static uint64_t lfsr = 0x5498173498171551UL;

static inline uint32_t
random_gen(uint8_t nbits)
{
    uint32_t val = 0;
    while (nbits--)
    {
        val = ((val << 1) | (lfsr & 1));
        uint32_t bits = (lfsr & POLYNOMIAL);   
        lfsr = ((lfsr >> 1) | ((uint64_t)(kernel::popcount(bits) & 1) << 63));
    }
    return val;
}

uint32_t
kernel::random(uint32_t first, uint32_t last)
{
    return first + (random_gen(32) % ((uint64_t)last - (uint64_t)first + 1));
}

void
kernel::random_seed(uint64_t val)
{
    lfsr = val;
    random_gen(64);
}
