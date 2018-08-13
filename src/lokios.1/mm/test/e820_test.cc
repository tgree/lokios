#include "../e820.h"
#include <tmock/tmock.h>
#include <vector>

using namespace kernel;
using namespace std;

static constexpr e820_map roel_16g_map = {
    20,
    {
        {0x0000000000000000,0x00009EC00,E820_TYPE_RAM,     1},
        {0x000000000009EC00,0x000001400,E820_TYPE_RESERVED,1},
        {0x00000000000E0000,0x000020000,E820_TYPE_RESERVED,1},
        {0x0000000000100000,0x003F00000,E820_TYPE_RAM,     1},
        {0x0000000004000000,0x00000A000,E820_TYPE_ACPI_NVS,1},
        {0x000000000400A000,0x005D76000,E820_TYPE_RAM,     1},
        {0x0000000009D80000,0x000280000,E820_TYPE_RESERVED,1},
        {0x000000000A000000,0x001000000,E820_TYPE_RAM,     1},
        {0x000000000B000000,0x000020000,E820_TYPE_RESERVED,1},
        {0x000000000B020000,0x0D22F5000,E820_TYPE_RAM,     1},
        {0x00000000DD315000,0x000118000,E820_TYPE_RESERVED,1},
        {0x00000000DD42D000,0x0000F7000,E820_TYPE_RAM,     1},
        {0x00000000DD524000,0x0003CA000,E820_TYPE_ACPI_NVS,1},
        {0x00000000DD8EE000,0x000D77000,E820_TYPE_RESERVED,1},
        {0x00000000DE665000,0x00099B000,E820_TYPE_RAM,     1},
        {0x0000000100000000,0x30F000000,E820_TYPE_RAM,     1},
        {0x00000000DF000000,0x001000000,E820_TYPE_RESERVED,1},
        {0x00000000F8000000,0x004000000,E820_TYPE_RESERVED,1},
        {0x00000000FD800000,0x000800000,E820_TYPE_RESERVED,1},
        {0x00000000FEA00000,0x000010000,E820_TYPE_RESERVED,1},
        // More follow but I can't read it from the photos.  I think it
        // is all type reserved.
    }
};

class tmock_test
{
    TMOCK_TEST(test_overlap_reserved)
    {
        static constexpr e820_map m = {
            3,
            {
                {0x0000000000000000,0x10000000,E820_TYPE_RAM,     1},
                {0x0000000020000000,0x10000000,E820_TYPE_RAM,     1},
                {0x0000000008000000,0x20000000,E820_TYPE_RESERVED,1},
            }
        };
        vector<region> ram_regions;
        get_e820_regions(&m,ram_regions,E820_TYPE_RAM_MASK);
        tmock::assert_equiv(ram_regions.size(),2U);

        vector<region> nonram_regions;
        get_e820_regions(&m,nonram_regions,~E820_TYPE_RAM_MASK);
        tmock::assert_equiv(nonram_regions.size(),1U);

        regions_remove(ram_regions,nonram_regions);
        tmock::assert_equiv(ram_regions.size(),2U);

        tmock::assert_equiv(ram_regions[0].first,0x00000000U);
        tmock::assert_equiv(ram_regions[0].last, 0x07FFFFFFU);
        tmock::assert_equiv(ram_regions[1].first,0x28000000U);
        tmock::assert_equiv(ram_regions[1].last, 0x2FFFFFFFU);
    }

    TMOCK_TEST(roel_ram_test)
    {
        vector<region> ram_regions;
        get_e820_regions(&roel_16g_map,ram_regions,E820_TYPE_RAM_MASK);
        tmock::assert_equiv(ram_regions.size(),8U);

        vector<region> nonram_regions;
        get_e820_regions(&roel_16g_map,nonram_regions,~E820_TYPE_RAM_MASK);
        tmock::assert_equiv(nonram_regions.size(),12U);

        regions_remove(ram_regions,nonram_regions);
        tmock::assert_equiv(ram_regions.size(),8U);

        tmock::assert_equiv(ram_regions[0].first,0x000000000U);
        tmock::assert_equiv(ram_regions[0].last, 0x00009EBFFU);
        tmock::assert_equiv(ram_regions[7].first,0x100000000U);
        tmock::assert_equiv(ram_regions[7].last, 0x40EFFFFFFU);
    }

    TMOCK_TEST_EXPECT_FAILURE_SHOULD_PASS(is_sorted_test)
    {
        static constexpr e820_map m = {
            5,
            {
                {0x9000,0x1000,E820_TYPE_RAM,1},
                {0x7000,0x1000,E820_TYPE_RAM,1},
                {0x5000,0x1000,E820_TYPE_RAM,1},
                {0x3000,0x1000,E820_TYPE_RAM,1},
                {0x1000,0x1000,E820_TYPE_RAM,1},
            }
        };

        vector<region> ram_regions;
        get_e820_regions(&m,ram_regions,E820_TYPE_RAM_MASK);
        for (size_t i=1; i<ram_regions.size(); ++i)
            TASSERT(ram_regions[i-1].first < ram_regions[i].first);
    }

    TMOCK_TEST_EXPECT_FAILURE_SHOULD_PASS(is_compressed_test)
    {
        static constexpr e820_map m = {
            5,
            {
                {0x5000,0x1000,E820_TYPE_RAM,1},
                {0x4000,0x2000,E820_TYPE_RAM,1},
                {0x3000,0x3000,E820_TYPE_RAM,1},
                {0x2000,0x4000,E820_TYPE_RAM,1},
                {0x1000,0x5000,E820_TYPE_RAM,1},
            }
        };

        vector<region> ram_regions;
        get_e820_regions(&m,ram_regions,E820_TYPE_RAM_MASK);
        tmock::assert_equiv(ram_regions.size(),1U);
        tmock::assert_equiv(ram_regions[0].first,0x1000U);
        tmock::assert_equiv(ram_regions[0].last,0x5FFFU);
    }

    TMOCK_TEST_EXPECT_FAILURE_SHOULD_PASS(reverse_chain_test)
    {
        static constexpr e820_map m = {
            5,
            {
                {0x5000,0x1000,E820_TYPE_RAM,1},
                {0x4000,0x1000,E820_TYPE_RAM,1},
                {0x3000,0x1000,E820_TYPE_RAM,1},
                {0x2000,0x1000,E820_TYPE_RAM,1},
                {0x1000,0x1000,E820_TYPE_RAM,1},
            }
        };

        vector<region> ram_regions;
        get_e820_regions(&m,ram_regions,E820_TYPE_RAM_MASK);
        tmock::assert_equiv(ram_regions.size(),1U);
        tmock::assert_equiv(ram_regions[0].first,0x1000U);
        tmock::assert_equiv(ram_regions[0].last,0x5FFFU);
    }
};

TMOCK_MAIN();
