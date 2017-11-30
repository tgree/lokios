#include "../console.h"
#include "../kernel_args.h"
#include "tmock/tmock.h"

static uint16_t conbuf[80*25];
static const kernel_args args = {0,(uintptr_t)conbuf};
const kernel_args* kargs = &args;

void
panic(const char* s) noexcept
{
    tmock::abort(s);
}

TMOCK_TEST(test_screen_initialized)
{
    for (uint16_t v : conbuf)
        tmock::assert_equiv(v,0x1F20);
}

TMOCK_TEST(test_putc)
{
    tmock::assert_equiv(conbuf[0],0x1F20);
    vga._putc('T');
    tmock::assert_equiv(conbuf[0],0x1F00 | 'T');
}

TMOCK_MAIN();
