#include "../vm.h"
#include <tmock/tmock.h>

class tmock_test
{
    TMOCK_TEST(test_empty_vm_throws)
    {
        vmm::machine vm;
        try
        {
            vm.read8(0x1000);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x1000U);
        }
    }

    TMOCK_TEST(test_adjacent_fine)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        auto* bank1 = vm.make_ram(0x2000,0x1000);
        vm.insert_device(bank0);
        vm.insert_device(bank1);
        for (size_t i=0; i<0x2000; ++i)
            vm.write8((uint8_t)i,0x1000+i);
        for (size_t i=0; i<0x2000; ++i)
            tmock::assert_equiv(vm.read8(0x1000+i),(uint8_t)i);
    }

    TMOCK_TEST(test_overlap_back_throw)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        auto* bank1 = vm.make_ram(0x1FFF,0x1000);
        vm.insert_device(bank0);
        try
        {
            vm.insert_device(bank1);
            tmock::abort("device_conflict_error expected");
        }
        catch (const vmm::device_conflict_error& e)
        {
            vm.delete_device(bank1);
        }
    }

    TMOCK_TEST(test_overlap_front_throw)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        auto* bank1 = vm.make_ram(0x1FFF,0x1000);
        vm.insert_device(bank1);
        try
        {
            vm.insert_device(bank0);
            tmock::abort("device_conflict_error expected");
        }
        catch (const vmm::device_conflict_error& e)
        {
            vm.delete_device(bank0);
        }
    }

    TMOCK_TEST(test_endian_correct)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        vm.insert_device(bank0);

        vm.write64(0x0123456789ABCDEF,0x1000);
        tmock::assert_equiv(vm.read8(0x1000),0xEFU);
        tmock::assert_equiv(vm.read8(0x1001),0xCDU);
        tmock::assert_equiv(vm.read8(0x1002),0xABU);
        tmock::assert_equiv(vm.read8(0x1003),0x89U);
        tmock::assert_equiv(vm.read8(0x1004),0x67U);
        tmock::assert_equiv(vm.read8(0x1005),0x45U);
        tmock::assert_equiv(vm.read8(0x1006),0x23U);
        tmock::assert_equiv(vm.read8(0x1007),0x01U);

        vm.write32(0x01234567,0x1000);
        tmock::assert_equiv(vm.read8(0x1000),0x67U);
        tmock::assert_equiv(vm.read8(0x1001),0x45U);
        tmock::assert_equiv(vm.read8(0x1002),0x23U);
        tmock::assert_equiv(vm.read8(0x1003),0x01U);

        vm.write16(0x0123,0x1000);
        tmock::assert_equiv(vm.read8(0x1000),0x23U);
        tmock::assert_equiv(vm.read8(0x1001),0x01U);
    }

    TMOCK_TEST(test_overlap_front_16_throw)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        vm.insert_device(bank0);
        try
        {
            vm.write16(0,0x0FFF);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x0FFFU);
        }
    }

    TMOCK_TEST(test_overlap_front_32_throw)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        vm.insert_device(bank0);
        try
        {
            vm.write32(0,0x0FFD);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x0FFDU);
        }
    }

    TMOCK_TEST(test_overlap_front_64_throw)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        vm.insert_device(bank0);
        try
        {
            vm.write64(0,0x0FF9);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x0FF9U);
        }
    }

    TMOCK_TEST(test_overlap_back_16_throw)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        vm.insert_device(bank0);
        try
        {
            vm.write16(0,0x1FFF);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x2000U);
        }
    }

    TMOCK_TEST(test_overlap_back_32_throw)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        vm.insert_device(bank0);
        try
        {
            vm.write32(0,0x1FFD);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x2000U);
        }
    }

    TMOCK_TEST(test_overlap_back_64_throw)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        vm.insert_device(bank0);
        try
        {
            vm.write64(0,0x1FF9);
            tmock::abort("bus_error expected");
        }
        catch (const vmm::bus_error& e)
        {
            tmock::assert_equiv(e.vm_addr,0x2000U);
        }
    }

    TMOCK_TEST(test_write_read_works)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        vm.insert_device(bank0);

        vm.write8(0x01,0x1234);
        tmock::assert_equiv(vm.read8(0x1234),0x01U);

        vm.write16(0x2345,0x1234);
        tmock::assert_equiv(vm.read16(0x1234),0x2345U);

        vm.write32(0x6789ABCD,0x1234);
        tmock::assert_equiv(vm.read32(0x1234),0x6789ABCDU);

        vm.write64(0xEF0123456789ABCD,0x1234);
        tmock::assert_equiv(vm.read64(0x1234),0xEF0123456789ABCDU);
    }

    TMOCK_TEST(test_write_read_double_banks_works)
    {
        vmm::machine vm;
        auto* bank0 = vm.make_ram(0x1000,0x1000);
        auto* bank1 = vm.make_ram(0x2000,0x1000);
        vm.insert_device(bank0);
        vm.insert_device(bank1);

        vm.write16(0x2345,0x1FFF);
        tmock::assert_equiv(vm.read16(0x1FFF),0x2345U);

        vm.write32(0x6789ABCD,0x1FFF);
        tmock::assert_equiv(vm.read32(0x1FFF),0x6789ABCDU);

        vm.write64(0xEF0123456789ABCD,0x1FFD);
        tmock::assert_equiv(vm.read64(0x1FFD),0xEF0123456789ABCDU);
    }
};

TMOCK_MAIN();
