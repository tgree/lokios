#include "../buddy_allocator.h"
#include <tmock/tmock.h>

using kernel::_kassert;
using kernel::buddy_allocator;
using kernel::buddy_allocator_params;

class tmock_test
{
    TMOCK_TEST(test_base4_len3_params)
    {
        buddy_allocator_params ba(4*PAGE_SIZE,3*PAGE_SIZE);

        tmock::assert_equiv(ba.L,4*PAGE_SIZE);
        tmock::assert_equiv(ba.B,4*PAGE_SIZE);
        tmock::assert_equiv(ba.E,8*PAGE_SIZE);
        tmock::assert_equiv(ba.M,2UL);

        tmock::assert_equiv(ba.get_inuse_bitmask_size(),1UL);

        tmock::assert_equiv(ba.index_for_paddr(4*PAGE_SIZE,0),0UL);
        tmock::assert_equiv(ba.index_for_paddr(5*PAGE_SIZE,0),1UL);
        tmock::assert_equiv(ba.index_for_paddr(6*PAGE_SIZE,0),2UL);
        tmock::assert_equiv(ba.index_for_paddr(7*PAGE_SIZE,0),3UL);

        tmock::assert_equiv(ba.index_for_paddr(4*PAGE_SIZE,1),4UL);
        tmock::assert_equiv(ba.index_for_paddr(6*PAGE_SIZE,1),5UL);

        tmock::assert_equiv(ba.index_for_paddr(4*PAGE_SIZE,2),6UL);
    }

    TMOCK_TEST(test_base4_len2_params)
    {
        buddy_allocator_params ba(4*PAGE_SIZE,2*PAGE_SIZE);

        tmock::assert_equiv(ba.L,2*PAGE_SIZE);
        tmock::assert_equiv(ba.B,4*PAGE_SIZE);
        tmock::assert_equiv(ba.E,6*PAGE_SIZE);
        tmock::assert_equiv(ba.M,1UL);

        tmock::assert_equiv(ba.get_inuse_bitmask_size(),1UL);

        tmock::assert_equiv(ba.index_for_paddr(4*PAGE_SIZE,0),0UL);
        tmock::assert_equiv(ba.index_for_paddr(5*PAGE_SIZE,0),1UL);
        tmock::assert_equiv(ba.index_for_paddr(4*PAGE_SIZE,1),2UL);
    }

    TMOCK_TEST(test_base5_len2_params)
    {
        buddy_allocator_params ba(5*PAGE_SIZE,2*PAGE_SIZE);

        tmock::assert_equiv(ba.L,2*PAGE_SIZE);
        tmock::assert_equiv(ba.B,4*PAGE_SIZE);
        tmock::assert_equiv(ba.E,8*PAGE_SIZE);
        tmock::assert_equiv(ba.M,2UL);

        tmock::assert_equiv(ba.get_inuse_bitmask_size(),1UL);

        tmock::assert_equiv(ba.index_for_paddr(4*PAGE_SIZE,0),0UL);
        tmock::assert_equiv(ba.index_for_paddr(5*PAGE_SIZE,0),1UL);
        tmock::assert_equiv(ba.index_for_paddr(6*PAGE_SIZE,0),2UL);
        tmock::assert_equiv(ba.index_for_paddr(7*PAGE_SIZE,0),3UL);

        tmock::assert_equiv(ba.index_for_paddr(4*PAGE_SIZE,1),4UL);
        tmock::assert_equiv(ba.index_for_paddr(6*PAGE_SIZE,1),5UL);

        tmock::assert_equiv(ba.index_for_paddr(4*PAGE_SIZE,2),6UL);
    }

    TMOCK_TEST(test_base1000_len64_params)
    {
        buddy_allocator_params ba(1000*PAGE_SIZE,64*PAGE_SIZE);

        tmock::assert_equiv(ba.L,64*PAGE_SIZE);
        tmock::assert_equiv(ba.B,960*PAGE_SIZE);
        tmock::assert_equiv(ba.E,1088*PAGE_SIZE);
        tmock::assert_equiv(ba.M,7UL);

        tmock::assert_equiv(ba.get_inuse_bitmask_size(),16UL);

        tmock::assert_equiv(ba.index_for_paddr(1000*PAGE_SIZE,0),40U);
        tmock::assert_equiv(ba.index_for_paddr(1063*PAGE_SIZE,0),103U);
        tmock::assert_equiv(ba.index_for_paddr(1000*PAGE_SIZE,1),148U);
        tmock::assert_equiv(ba.index_for_paddr(1062*PAGE_SIZE,1),179U);
        tmock::assert_equiv(ba.index_for_paddr(1000*PAGE_SIZE,2),202U);
        tmock::assert_equiv(ba.index_for_paddr(1060*PAGE_SIZE,2),217U);
        tmock::assert_equiv(ba.index_for_paddr(1000*PAGE_SIZE,3),229U);
        tmock::assert_equiv(ba.index_for_paddr(1024*PAGE_SIZE,7),254U);
    }

    TMOCK_TEST(test_base4_len3_toggle)
    {
        KASSERT(buddy_allocator_params(4*PAGE_SIZE,3*PAGE_SIZE)
                .get_inuse_bitmask_size() == 1);

        uint8_t         inuse_bitmask[1] = {0};
        buddy_allocator ba(4*PAGE_SIZE,3*PAGE_SIZE,inuse_bitmask);
        tmock::assert_equiv(inuse_bitmask[0],0xFF);

        ba.toggle_inuse_ppfn_bit(4,0);
        tmock::assert_equiv(inuse_bitmask[0],0xFE);
        ba.toggle_inuse_ppfn_bit(5,0);
        tmock::assert_equiv(inuse_bitmask[0],0xFF);
        ba.toggle_inuse_ppfn_bit(6,0);
        tmock::assert_equiv(inuse_bitmask[0],0xFD);
        ba.toggle_inuse_ppfn_bit(7,0);
        tmock::assert_equiv(inuse_bitmask[0],0xFF);
        ba.toggle_inuse_ppfn_bit(4,1);
        tmock::assert_equiv(inuse_bitmask[0],0xFB);
        ba.toggle_inuse_ppfn_bit(6,1);
        tmock::assert_equiv(inuse_bitmask[0],0xFF);
        ba.toggle_inuse_ppfn_bit(4,2);
        tmock::assert_equiv(inuse_bitmask[0],0xF7);
    }

    TMOCK_TEST(test_len3_4mod8_free_pages)
    {
        uint8_t inuse_bitmask[1] = {0};

        void* p;
        kassert(posix_memalign(&p,8*PAGE_SIZE,8*PAGE_SIZE) == 0);
        dma_addr64 d = kernel::virt_to_phys(p);
        kassert((d & PAGE_OFFSET_MASK) == 0);

        buddy_allocator ba(d,3*PAGE_SIZE,inuse_bitmask);

        tmock::assert_equiv(ba.nfree_pages,0UL);
        ba.free_pages(d + 0*PAGE_SIZE,0);
        tmock::assert_equiv(ba.nfree_pages,1UL);
        tmock::assert_equiv(ba.order_list[0].size(),1UL);
        kassert((void*)ba.order_list[0].first_link() == p);

        ba.free_pages(d + 1*PAGE_SIZE,0);
        tmock::assert_equiv(ba.nfree_pages,2UL);
        tmock::assert_equiv(ba.order_list[0].size(),0UL);
        tmock::assert_equiv(ba.order_list[1].size(),1UL);
        kassert((void*)ba.order_list[1].first_link() == p);

        ba.free_pages(d + 2*PAGE_SIZE,0);
        tmock::assert_equiv(ba.nfree_pages,3UL);
        tmock::assert_equiv(ba.order_list[0].size(),1UL);
        tmock::assert_equiv(ba.order_list[1].size(),1UL);
        kassert((void*)ba.order_list[0].first_link() ==
                (void*)(d + 2*PAGE_SIZE));
        kassert((void*)ba.order_list[1].first_link() == p);
    }

    TMOCK_TEST_EXPECT_FAILURE(test_len3_4mod8_free_underflow)
    {
        uint8_t inuse_bitmask[1] = {0};

        void* p;
        kassert(posix_memalign(&p,8*PAGE_SIZE,8*PAGE_SIZE) == 0);
        dma_addr64 d = kernel::virt_to_phys(p);
        buddy_allocator ba(d,3*PAGE_SIZE,inuse_bitmask);

        ba.free_pages(d - 1*PAGE_SIZE,0);
    }

    TMOCK_TEST_EXPECT_FAILURE(test_len3_4mod8_free_overflow)
    {
        uint8_t inuse_bitmask[1] = {0};

        void* p;
        kassert(posix_memalign(&p,8*PAGE_SIZE,8*PAGE_SIZE) == 0);
        dma_addr64 d = kernel::virt_to_phys(p);
        buddy_allocator ba(d,3*PAGE_SIZE,inuse_bitmask);

        ba.free_pages(d + 3*PAGE_SIZE,0);
    }

    TMOCK_TEST(test_len3_4mod8_alloc_pages)
    {
        uint8_t inuse_bitmask[1] = {0};

        void* p;
        kassert(posix_memalign(&p,8*PAGE_SIZE,8*PAGE_SIZE) == 0);
        dma_addr64 d = kernel::virt_to_phys(p);

        buddy_allocator ba(d,3*PAGE_SIZE,inuse_bitmask);
        kassert(ba.nfree_pages == 0);
        ba.free_pages(d + 0*PAGE_SIZE,0);
        kassert(ba.nfree_pages == 1);
        ba.free_pages(d + 1*PAGE_SIZE,0);
        kassert(ba.nfree_pages == 2);
        ba.free_pages(d + 2*PAGE_SIZE,0);
        kassert(ba.nfree_pages == 3);

        dma_addr64 addrs[3];
        addrs[0] = ba.alloc_pages(0);
        kassert(ba.nfree_pages == 2);
        addrs[1] = ba.alloc_pages(0);
        kassert(ba.nfree_pages == 1);
        addrs[2] = ba.alloc_pages(0);
        kassert(ba.nfree_pages == 0);
        kassert(addrs[0] == d + 2*PAGE_SIZE);
        kassert(addrs[1] == d + 0*PAGE_SIZE);
        kassert(addrs[2] == d + 1*PAGE_SIZE);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());

        ba.free_pages(addrs[0],0);
        kassert(ba.nfree_pages == 1);
        ba.free_pages(addrs[1],0);
        kassert(ba.nfree_pages == 2);
        ba.free_pages(addrs[2],0);
        kassert(ba.nfree_pages == 3);

        addrs[0] = ba.alloc_pages(0);
        kassert(ba.nfree_pages == 2);
        addrs[1] = ba.alloc_pages(1);
        kassert(ba.nfree_pages == 0);
        kassert(addrs[0] == d + 2*PAGE_SIZE);
        kassert(addrs[1] == d + 0*PAGE_SIZE);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());
    }

    TMOCK_TEST(test_len3_4mod8_alloc_pages_oom_fails)
    {
        uint8_t inuse_bitmask[1] = {0};

        void* p;
        kassert(posix_memalign(&p,8*PAGE_SIZE,8*PAGE_SIZE) == 0);
        dma_addr64 d = kernel::virt_to_phys(p);

        buddy_allocator ba(d,3*PAGE_SIZE,inuse_bitmask);
        ba.free_pages(d + 0*PAGE_SIZE,0);
        ba.free_pages(d + 1*PAGE_SIZE,0);
        ba.free_pages(d + 2*PAGE_SIZE,0);
        ba.alloc_pages(0);
        ba.alloc_pages(0);
        ba.alloc_pages(0);
        try
        {
            ba.alloc_pages(0);
            tmock::abort("didn't throw buddy_allocator_oom_exception");
        }
        catch (kernel::buddy_allocator_oom_exception&)
        {
        }
    }

    TMOCK_TEST(test_len8_0mod8_alloc_pages)
    {
        uint8_t inuse_bitmask[1] = {0};

        void* p;
        kassert(posix_memalign(&p,8*PAGE_SIZE,8*PAGE_SIZE) == 0);
        dma_addr64 d = kernel::virt_to_phys(p);

        buddy_allocator ba(d,8*PAGE_SIZE,inuse_bitmask);
        kassert(ba.params.get_inuse_bitmask_size() == 1);
        for (size_t i=0; i<8; ++i)
        {
            kassert(ba.nfree_pages == i);
            ba.free_pages(d + i*PAGE_SIZE,0);
            kassert(ba.nfree_pages == i+1);
        }
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].empty());
        kassert(ba.order_list[3].size() == 1);

        kassert(ba.alloc_pages(0) == d + 0*PAGE_SIZE);
        kassert(ba.nfree_pages == 7);
        memset((void*)(d + 0*PAGE_SIZE),0,PAGE_SIZE);
        kassert(ba.order_list[0].size() == 1);
        kassert(ba.order_list[1].size() == 1);
        kassert(ba.order_list[2].size() == 1);
        kassert(ba.order_list[3].empty());

        ba.free_pages(d,0);
        kassert(ba.nfree_pages == 8);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].empty());
        kassert(ba.order_list[3].size() == 1);

        kassert(ba.alloc_pages(0) == d + 0*PAGE_SIZE);
        kassert(ba.nfree_pages == 7);
        memset((void*)(d + 0*PAGE_SIZE),0,PAGE_SIZE);
        kassert(ba.alloc_pages(0) == d + 1*PAGE_SIZE);
        kassert(ba.nfree_pages == 6);
        memset((void*)(d + 1*PAGE_SIZE),0,PAGE_SIZE);
        ba.free_pages(d,0);
        kassert(ba.nfree_pages == 7);
        kassert(ba.order_list[0].size() == 1);
        kassert(ba.order_list[1].size() == 1);
        kassert(ba.order_list[2].size() == 1);
        kassert(ba.order_list[3].empty());

        ba.free_pages(d + 1*PAGE_SIZE,0);
        kassert(ba.nfree_pages == 8);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].empty());
        kassert(ba.order_list[3].size() == 1);

        kassert(ba.alloc_pages(0) == d + 0*PAGE_SIZE);
        kassert(ba.nfree_pages == 7);
        memset((void*)(d + 0*PAGE_SIZE),0,PAGE_SIZE);
        kassert(ba.alloc_pages(0) == d + 1*PAGE_SIZE);
        kassert(ba.nfree_pages == 6);
        memset((void*)(d + 1*PAGE_SIZE),0,PAGE_SIZE);
        kassert(ba.alloc_pages(0) == d + 2*PAGE_SIZE);
        kassert(ba.nfree_pages == 5);
        memset((void*)(d + 2*PAGE_SIZE),0,PAGE_SIZE);
        kassert(ba.order_list[0].size() == 1);
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].size() == 1);
        kassert(ba.order_list[3].empty());

        ba.free_pages(d + 1*PAGE_SIZE,0);
        kassert(ba.nfree_pages == 6);
        kassert(ba.order_list[0].size() == 2);
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].size() == 1);
        kassert(ba.order_list[3].empty());

        ba.free_pages(d,0);
        kassert(ba.nfree_pages == 7);
        kassert(ba.order_list[0].size() == 1);
        kassert(ba.order_list[1].size() == 1);
        kassert(ba.order_list[2].size() == 1);
        kassert(ba.order_list[3].empty());

        ba.free_pages(d + 2*PAGE_SIZE,0);
        kassert(ba.nfree_pages == 8);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].empty());
        kassert(ba.order_list[3].size() == 1);

        kassert(ba.alloc_pages(1) == d + 0*PAGE_SIZE);
        kassert(ba.nfree_pages == 6);
        memset((void*)(d + 0*PAGE_SIZE),0,2*PAGE_SIZE);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].size() == 1);
        kassert(ba.order_list[2].size() == 1);
        kassert(ba.order_list[3].empty());

        ba.free_pages(d + 0*PAGE_SIZE,1);
        kassert(ba.nfree_pages == 8);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].empty());
        kassert(ba.order_list[3].size() == 1);

        kassert(ba.alloc_pages(2) == d + 0*PAGE_SIZE);
        kassert(ba.nfree_pages == 4);
        memset((void*)(d + 0*PAGE_SIZE),0,4*PAGE_SIZE);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].size() == 1);
        kassert(ba.order_list[3].empty());

        ba.free_pages(d + 0*PAGE_SIZE,2);
        kassert(ba.nfree_pages == 8);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].empty());
        kassert(ba.order_list[3].size() == 1);

        kassert(ba.alloc_pages(3) == d + 0*PAGE_SIZE);
        kassert(ba.nfree_pages == 0);
        memset((void*)(d + 0*PAGE_SIZE),0,8*PAGE_SIZE);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].empty());
        kassert(ba.order_list[3].empty());

        ba.free_pages(d + 0*PAGE_SIZE,3);
        kassert(ba.nfree_pages == 8);
        kassert(ba.order_list[0].empty());
        kassert(ba.order_list[1].empty());
        kassert(ba.order_list[2].empty());
        kassert(ba.order_list[3].size() == 1);
    }
};

TMOCK_MAIN();
