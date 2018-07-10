#include "../page_table.h"
#include "../mm.h"
#include <tmock/tmock.h>
#include <hdr/compiler.h>
#include <string.h>

extern size_t page_alloc_count;
extern size_t page_free_count;

struct pte_check
{
    size_t      levels;
    size_t      vaddr;
    uint64_t    mask;
    uint64_t    val;
};

class tmock_test
{
    template<size_t N>
    static void check_ptes(const kernel::page_table& pt,
        const pte_check (&positions)[N])
    {
        for (const pte_check& pos : positions)
        {
            uint64_t* page = pt.cr3;
            uint64_t vaddr = pos.vaddr;
            size_t index   = ((vaddr >> 39) & 0x1FF);
            for (size_t i = 0; i != pos.levels - 1; ++i)
            {
                page    = (uint64_t*)(page[index] & PAGE_PADDR_MASK);
                vaddr <<= 9;
                index   = ((vaddr >> 39) & 0x1FF);
            }
            uint64_t pte = page[index];
            tmock::assert_equiv(pte & pos.mask,pos.val);

            if (pos.val & PAGE_FLAG_USER_PAGE)
            {
                tmock::assert_equiv(pt.xlate((void*)(pos.vaddr + 1234)),
                                    (pos.val & PAGE_PADDR_MASK) + 1234);
            }
        }
    }

    TMOCK_TEST(test_page_table_iterator_all_empty_page_works)
    {
        auto p = (uint64_t*)kernel::page_zalloc();

        size_t count = 0;
        for (auto pte __UNUSED__ : kernel::page_table_iterator(p,0,0))
            ++count;
        TASSERT(count == 512);

        kernel::page_free(p);
    }

    TMOCK_TEST(test_page_table_present_iterator_empty_page_works)
    {
        auto p = (uint64_t*)kernel::page_zalloc();

        size_t count = 0;
        for (auto pte __UNUSED__ : kernel::page_table_present_iterator(p))
            ++count;
        TASSERT(count == 0);

        kernel::page_free(p);
    }

    TMOCK_TEST(test_page_table_leaf_iterator_empty_page_works)
    {
        auto p = (uint64_t*)kernel::page_zalloc();

        size_t count = 0;
        for (auto pte __UNUSED__ : kernel::page_table_leaf_iterator(p))
            ++count;
        TASSERT(count == 0);

        kernel::page_free(p);
    }

    TMOCK_TEST(test_page_table_nonleaf_iterator_empty_page_works)
    {
        auto p = (uint64_t*)kernel::page_zalloc();

        size_t count = 0;
        for (auto pte __UNUSED__ : kernel::page_table_nonleaf_iterator(p))
            ++count;
        TASSERT(count == 0);

        kernel::page_free(p);
    }

    TMOCK_TEST(test_empty_page_table_doesnt_leak)
    {
        TASSERT(page_alloc_count == 0);
        TASSERT(page_free_count == 0);

        {
            kernel::page_table pt;
            TASSERT(page_alloc_count == 1);
            TASSERT(page_free_count == 0);
        }

        TASSERT(page_alloc_count == 1);
        TASSERT(page_free_count == 1);
    }

    TMOCK_TEST(test_4k_level_fields_work)
    {
        kernel::page_table pt;
        pt.map_4k_page((void*)0,0,PAGE_FLAG_WRITEABLE);
        TASSERT((pt.cr3[0] & ~PAGE_PADDR_MASK) == 0x0000000000000003UL);
        uint64_t* l2 = (uint64_t*)(pt.cr3[0] & PAGE_PADDR_MASK);
        TASSERT((l2[0] & ~PAGE_PADDR_MASK) == 0x1000000000000003UL);
        uint64_t* l3 = (uint64_t*)(l2[0] & PAGE_PADDR_MASK);
        TASSERT((l3[0] & ~PAGE_PADDR_MASK) == 0x2000000000000003UL);
        uint64_t* l4 = (uint64_t*)(l3[0] & PAGE_PADDR_MASK);
        TASSERT(l4[0] == 0x3000000000000083UL);
    }

    TMOCK_TEST(test_2m_level_fields_work)
    {
        kernel::page_table pt;
        pt.map_2m_page((void*)0,0,PAGE_FLAG_WRITEABLE);
        TASSERT((pt.cr3[0] & ~PAGE_PADDR_MASK) == 0x0000000000000003UL);
        uint64_t* l2 = (uint64_t*)(pt.cr3[0] & PAGE_PADDR_MASK);
        TASSERT((l2[0] & ~PAGE_PADDR_MASK) == 0x1000000000000003UL);
        uint64_t* l3 = (uint64_t*)(l2[0] & PAGE_PADDR_MASK);
        TASSERT(l3[0] == 0x2000000000000083UL);
    }

    TMOCK_TEST(test_1g_level_fields_work)
    {
        kernel::page_table pt;
        pt.map_1g_page((void*)0,0,PAGE_FLAG_WRITEABLE);
        TASSERT((pt.cr3[0] & ~PAGE_PADDR_MASK) == 0x0000000000000003UL);
        uint64_t* l2 = (uint64_t*)(pt.cr3[0] & PAGE_PADDR_MASK);
        TASSERT(l2[0] == 0x1000000000000083UL);
    }

    TMOCK_TEST(test_canonical_form_works)
    {
        kernel::page_table pt;
        pt.map_4k_page((void*)0xFFFF800000000000UL,0UL,PAGE_FLAG_WRITEABLE);
    }

    TMOCK_TEST_EXPECT_FAILURE(test_canonical_form_requirement)
    {
        kernel::page_table pt;
        pt.map_4k_page((void*)0x8000000000000000UL,0UL,PAGE_FLAG_WRITEABLE);
    }

    TMOCK_TEST(test_random_map_4k_works)
    {
        {
            kernel::page_table pt;
            pt.map_4k_page((void*)0x0000123456789000,0x000000ABCDEFF000,0);
            TASSERT(page_alloc_count == 4);
            TASSERT(page_free_count == 0);

            pte_check ptes[] = {
                {1,0x0000123456789000,~PAGE_PADDR_MASK,  0x0000000000000003UL},
                {2,0x0000123456789000,~PAGE_PADDR_MASK,  0x1000000000000003UL},
                {3,0x0000123456789000,~PAGE_PADDR_MASK,  0x2000000000000003UL},
                {4,0x0000123456789000,0xFFFFFFFFFFFFFFFF,0x300000ABCDEFF081UL},
            };
            check_ptes(pt,ptes);
        }
        TASSERT(page_alloc_count == 4);
        TASSERT(page_free_count == 4);
    }

    TMOCK_TEST(test_random_unmap_4k_works)
    {
        {
            kernel::page_table pt;
            pt.map_4k_page((void*)0x0000123456789000,0x000000ABCDEFF000,0);
            texpect("kernel::tlb_shootdown");
            pt.unmap_page((void*)0x0000123456789ABC);
            TASSERT(page_alloc_count == 4);
            TASSERT(page_free_count == 0);

            pte_check ptes[] = {
                {1,0x0000123456789000,~PAGE_PADDR_MASK,  0x0000000000000003UL},
                {2,0x0000123456789000,~PAGE_PADDR_MASK,  0x1000000000000003UL},
                {3,0x0000123456789000,~PAGE_PADDR_MASK,  0x2000000000000003UL},
                {4,0x0000123456789000,0xFFFFFFFFFFFFFFFF,0x0000000000000000UL},
            };
            check_ptes(pt,ptes);
        }
        TASSERT(page_alloc_count == 4);
        TASSERT(page_free_count == 4);
    }

    TMOCK_TEST(test_random_map_2m_works)
    {
        {
            kernel::page_table pt;
            pt.map_2m_page((void*)0x00002468ACE00000,0x000000ABCDE00000,0);
            TASSERT(page_alloc_count == 3);
            TASSERT(page_free_count == 0);

            pte_check ptes[] = {
                {1,0x00002468ACE00000,~PAGE_PADDR_MASK,  0x0000000000000003UL},
                {2,0x00002468ACE00000,~PAGE_PADDR_MASK,  0x1000000000000003UL},
                {3,0x00002468ACE00000,0xFFFFFFFFFFFFFFFF,0x200000ABCDE00081UL},
            };
            check_ptes(pt,ptes);
        }
        TASSERT(page_alloc_count == 3);
        TASSERT(page_free_count == 3);
    }

    TMOCK_TEST(test_random_unmap_2m_works)
    {
        {
            kernel::page_table pt;
            pt.map_2m_page((void*)0x00002468ACE00000,0x000000ABCDE00000,0);
            texpect("kernel::tlb_shootdown");
            pt.unmap_page((void*)0x00002468ACE13579);
            TASSERT(page_alloc_count == 3);
            TASSERT(page_free_count == 0);

            pte_check ptes[] = {
                {1,0x00002468ACE00000,~PAGE_PADDR_MASK,  0x0000000000000003UL},
                {2,0x00002468ACE00000,~PAGE_PADDR_MASK,  0x1000000000000003UL},
                {3,0x00002468ACE00000,0xFFFFFFFFFFFFFFFF,0x0000000000000000UL},
            };
            check_ptes(pt,ptes);
        }
        TASSERT(page_alloc_count == 3);
        TASSERT(page_free_count == 3);
    }

    TMOCK_TEST(test_random_map_1g_works)
    {
        {
            kernel::page_table pt;
            pt.map_1g_page((void*)0x00000000C0000000UL,0x0000000140000000UL,
                            PAGE_FLAG_WRITEABLE);
            TASSERT(page_alloc_count == 2);
            TASSERT(page_free_count == 0);

            pte_check ptes[] = {
                {1,0x00000000C0000000,~PAGE_PADDR_MASK,  0x0000000000000003UL},
                {2,0x00000000C0000000,0xFFFFFFFFFFFFFFFF,0x1000000140000083UL},
            };
            check_ptes(pt,ptes);
        }
        TASSERT(page_alloc_count == 2);
        TASSERT(page_free_count == 2);
    }

    TMOCK_TEST(test_random_unmap_1g_works)
    {
        {
            kernel::page_table pt;
            pt.map_1g_page((void*)0x00000000C0000000UL,0x0000000140000000UL,
                            PAGE_FLAG_WRITEABLE);
            texpect("kernel::tlb_shootdown");
            pt.unmap_page((void*)0x00000000C1234567UL);
            TASSERT(page_alloc_count == 2);
            TASSERT(page_free_count == 0);

            pte_check ptes[] = {
                {1,0x00000000C0000000,~PAGE_PADDR_MASK,  0x0000000000000003UL},
                {2,0x00000000C0000000,0xFFFFFFFFFFFFFFFF,0x0000000000000000UL},
            };
            check_ptes(pt,ptes);
        }
        TASSERT(page_alloc_count == 2);
        TASSERT(page_free_count == 2);
    }

    TMOCK_TEST(test_mixed_mappings_on_shared_page_table_pages_work)
    {
        {
            kernel::page_table pt;
            pt.map_1g_page((void*)0x00000000C0000000UL,0x0000000140000000UL,
                            PAGE_FLAG_WRITEABLE);
            pt.map_2m_page((void*)0x0000000000400000,0x000000ABCDE00000,0);
            pt.map_2m_page((void*)0x0000000000200000,0x000000ABCE000000,0);
            pt.map_4k_page((void*)0x0000000000000000,0x0000000000001000,0);
            pt.map_4k_page((void*)0x0000000000001000,0x000000FFFFFFF000,0);
            TASSERT(page_alloc_count == 4);
            TASSERT(page_free_count == 0);

            pte_check ptes[] = {
                {1,0x00000000C0000000,~PAGE_PADDR_MASK,  0x0000000000000003UL},
                {1,0x0000000000400000,~PAGE_PADDR_MASK,  0x0000000000000003UL},
                {1,0x0000000000200000,~PAGE_PADDR_MASK,  0x0000000000000003UL},
                {1,0x0000000000000000,~PAGE_PADDR_MASK,  0x0000000000000003UL},
                {1,0x0000000000001000,~PAGE_PADDR_MASK,  0x0000000000000003UL},

                {2,0x00000000C0000000,0xFFFFFFFFFFFFFFFF,0x1000000140000083UL},
                {2,0x0000000000400000,~PAGE_PADDR_MASK,  0x1000000000000003UL},
                {2,0x0000000000200000,~PAGE_PADDR_MASK,  0x1000000000000003UL},
                {2,0x0000000000000000,~PAGE_PADDR_MASK,  0x1000000000000003UL},
                {2,0x0000000000001000,~PAGE_PADDR_MASK,  0x1000000000000003UL},

                {3,0x0000000000400000,0xFFFFFFFFFFFFFFFF,0x200000ABCDE00081UL},
                {3,0x0000000000200000,0xFFFFFFFFFFFFFFFF,0x200000ABCE000081UL},
                {3,0x0000000000000000,~PAGE_PADDR_MASK,  0x2000000000000003UL},
                {3,0x0000000000001000,~PAGE_PADDR_MASK,  0x2000000000000003UL},

                {4,0x0000000000000000,0xFFFFFFFFFFFFFFFF,0x3000000000001081UL},
                {4,0x0000000000001000,0xFFFFFFFFFFFFFFFF,0x300000FFFFFFF081UL},
            };
            check_ptes(pt,ptes);
        }
        TASSERT(page_alloc_count == 4);
        TASSERT(page_free_count == 4);
    }

    TMOCK_TEST(test_high_mapping_iterator_works)
    {
        {
            kernel::page_table pt;
            pt.map_4k_page((void*)0xFFFF800000000000,0x1000,
                           PAGE_FLAG_WRITEABLE);
            size_t count = 0;
            for (auto pte : kernel::page_table_leaf_iterator(pt.cr3))
            {
                ++count;
                TASSERT(pte.vaddr == (void*)0xFFFF800000000000);
                TASSERT(pte.get_paddr() == 0x1000);
            }
            TASSERT(count == 1);
        }
        TASSERT(page_alloc_count == 4);
        TASSERT(page_free_count == 4);
    }
};

TMOCK_MAIN();
