#include "a20_gate.h"
#include <stdint.h>

extern "C" void _a20_enable_int15h();

static uint16_t a20_sig;

static bool
is_a20_working()
{
    a20_sig = 0x1357;
    if (*(volatile uint16_t*)((uintptr_t)&a20_sig + 0x00100000) != 0x1357)
        return true;
    a20_sig = 0x2468;
    if (*(volatile uint16_t*)((uintptr_t)&a20_sig + 0x00100000) != 0x2468)
        return true;
    return false;
}
    
int
a20_enable()
{
    if (is_a20_working())
        return 0;

    _a20_enable_int15h();

    if (is_a20_working())
        return 0;
    return -1;
}
