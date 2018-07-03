#include "../region_set.h"
#include "tmock/tmock.h"
#include <vector>

void
dump_region_vector(const std::vector<kernel::region>& v)
{
    for (const auto& r : v)
        printf("0x%016lX:0x%016lX\n",r.first,r.last);
}

bool
are_vectors_equiv(const std::vector<kernel::region>& l,
                  const std::vector<kernel::region>& r)
{
    if (l.size() != r.size())
        return false;
    for (size_t i=0; i<l.size(); ++i)
        if (l[i] != r[i])
            return false;
    return true;
}

void
assert_vectors_equiv(const std::vector<kernel::region>& l,
                     const std::vector<kernel::region>& r)
{
    if (!are_vectors_equiv(l,r))
    {
        printf("Got regions:\n");
        dump_region_vector(l);
        printf("Expected regions:\n");
        dump_region_vector(r);
        kernel::panic();
    }
}

class tmock_test
{
    TMOCK_TEST(test_bochs_e820)
    {
        std::vector<kernel::region> nonram_regions = {
            kernel::region{0x0009F000,0x0009FFFF},
            kernel::region{0x000E8000,0x000FFFFF},
            kernel::region{0x01FF0000,0x01FFFFFF},
            kernel::region{0xFFFC0000,0xFFFFFFFF},
        };
        std::vector<kernel::region> ram_regions = {
            kernel::region{0x00000000,0x0009EFFF},
            kernel::region{0x00100000,0x01FEFFFF},
        };
        std::vector<kernel::region> expected_regions = {
            kernel::region{0x00000000,0x0009EFFF},
            kernel::region{0x00100000,0x01FEFFFF},
        };

        for (const auto& e : nonram_regions)
            kernel::region_remove(ram_regions,e.first,e.last);
        assert_vectors_equiv(ram_regions,expected_regions);
    }

    TMOCK_TEST(test_left_no_overlap)
    {
        std::vector<kernel::region> ram = {
            kernel::region{1,3}
        };
        std::vector<kernel::region> expected = {
            kernel::region{1,3}
        };
        kernel::region_remove(ram,0,0);
        assert_vectors_equiv(ram,expected);
    }

    TMOCK_TEST(test_left_partial_overlap)
    {
        std::vector<kernel::region> ram = {
            kernel::region{1,3}
        };
        std::vector<kernel::region> expected = {
            kernel::region{2,3}
        };
        kernel::region_remove(ram,0,1);
        assert_vectors_equiv(ram,expected);
    }

    TMOCK_TEST(test_encompass_overlap)
    {
        std::vector<kernel::region> ram = {
            kernel::region{1,3}
        };
        std::vector<kernel::region> expected = {};
        kernel::region_remove(ram,0,4);
        assert_vectors_equiv(ram,expected);
    }

    TMOCK_TEST(test_interior_overlap)
    {
        std::vector<kernel::region> ram = {
            kernel::region{1,3}
        };
        std::vector<kernel::region> expected = {
            kernel::region{1,1},
            kernel::region{3,3},
        };
        kernel::region_remove(ram,2,2);
        assert_vectors_equiv(ram,expected);
    }

    TMOCK_TEST(test_right_partial_overlap)
    {
        std::vector<kernel::region> ram = {
            kernel::region{1,3}
        };
        std::vector<kernel::region> expected = {
            kernel::region{1,2}
        };
        kernel::region_remove(ram,3,4);
        assert_vectors_equiv(ram,expected);
    }

    TMOCK_TEST(test_right_no_overlap)
    {
        std::vector<kernel::region> ram = {
            kernel::region{1,3}
        };
        std::vector<kernel::region> expected = {
            kernel::region{1,3}
        };
        kernel::region_remove(ram,4,5);
        assert_vectors_equiv(ram,expected);
    }

    TMOCK_TEST(test_complicated_overlap)
    {
        std::vector<kernel::region> ram = {
            kernel::region{1,3},
            kernel::region{6,7},
            kernel::region{11,13},
        };
        std::vector<kernel::region> expected = {
            kernel::region{1,2},
            kernel::region{12,13},
        };
        kernel::region_remove(ram,3,11);
        assert_vectors_equiv(ram,expected);
    }
};

TMOCK_MAIN();
