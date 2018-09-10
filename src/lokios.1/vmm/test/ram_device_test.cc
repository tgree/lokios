#include "../ram_device.h"
#include "../vm.h"
#include <tmock/tmock.h>

class tmock_test
{
    TMOCK_TEST(test_mem_zeroed)
    {
        constexpr size_t len = 128;
        vmm::ram_device rd(0x1000,len);
        for (size_t i=0; i<len; ++i)
            tmock::assert_equiv(rd.read8(0x1000+i),0U);
    }

    TMOCK_TEST(test_mem_count)
    {
        constexpr size_t len = 128;
        vmm::ram_device rd(0x1000,len);
        for (size_t i=0; i<len; ++i)
            rd.write8(i,0x1000+i);
        for (size_t i=0; i<len; ++i)
            tmock::assert_equiv(rd.read8(0x1000+i),(uint8_t)i);
    }

    TMOCK_TEST(test_read8_low_throws)
    {
        vmm::ram_device rd(0x1000,256);
        try
        {
            rd.read8(0x0FFF);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x0FFFU);
        }
    }

    TMOCK_TEST(test_read8_high_throws)
    {
        vmm::ram_device rd(0x1000,256);
        try
        {
            rd.read8(0x1100);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x1100U);
        }

        try
        {
            rd.read8(0x123456);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x123456U);
        }
    }

    TMOCK_TEST(test_write8_low_throws)
    {
        vmm::ram_device rd(0x1000,256);
        try
        {
            rd.write8(0,0x0FFF);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x0FFFU);
        }
    }

    TMOCK_TEST(test_write8_high_throws)
    {
        vmm::ram_device rd(0x1000,256);
        try
        {
            rd.write8(0,0x1100);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x1100U);
        }

        try
        {
            rd.write8(0,0x123456);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x123456U);
        }
    }
};

TMOCK_MAIN();
