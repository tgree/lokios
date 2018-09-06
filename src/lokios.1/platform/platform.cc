#include "platform.h"
#include "qemu.h"
#include "kern/console.h"
#include "acpi/tables.h"

using kernel::console::printf;

static kernel::platform* _platform;

struct dummy_platform : public kernel::platform
{
    virtual void _exit_guest(int status)
    {
        printf("dummy_platform::_exit_guest(%d) invoked!\n",status);
    }

    virtual void _reboot_guest()
    {
        auto* fadt = (kernel::fadt_table*)kernel::find_acpi_table(FADT_SIG);
        if (fadt && (fadt->flags & (1<<10)))
        {
            // ACPI reset register supported.
            if (fadt->reset_reg.addr_space_id == 1 &&
                fadt->reset_reg.register_bit_width == 8 &&
                fadt->reset_reg.register_bit_offset == 0 &&
                fadt->reset_reg.access_size == 1)
            {
                outb(fadt->reset_value,fadt->reset_reg.addr);
            }
        }
        printf("dummy_platform::_reboot_guest() invoked!\n");
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
kernel::reboot_guest()
{
    if (_platform)
        _platform->_reboot_guest();
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
