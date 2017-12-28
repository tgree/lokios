#include "platform.h"
#include "qemu.h"
#include "kernel/cpuid.h"
#include "kernel/console.h"

using kernel::console::printf;

static kernel::platform* _platform;

struct dummy_platform : public kernel::platform
{
    virtual void _exit_guest(int status)
    {
        printf("dummy_platform::_exit_guest(%d) invoked!\n",status);
    }

    dummy_platform():platform("dummy") {}
};

kernel::platform::platform(const char* name):
    name(name)
{
}

void
kernel::exit_guest(int status)
{
    if (_platform)
        _platform->_exit_guest(status);
    kernel::halt();
}

void
kernel::init_platform()
{
    uint32_t    brand[4];
    cpuid(0x80000002,0,brand);
    if (brand[0] == QEMU_BRAND_SIGNATURE)
        _platform = new qemu_platform;
    else
        _platform = new dummy_platform;

    printf("Platform: %s\n",_platform->name);
}
