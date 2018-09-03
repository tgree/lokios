#include "../kstring.h"
#include "tmock/tmock.h"
#include <string>

extern size_t buddy_alloc_count;
extern size_t buddy_free_count;

class tmock_test
{
    TMOCK_TEST(test_kstring_basic)
    {
        tmock::assert_equiv(buddy_alloc_count,0U);
        tmock::assert_equiv(buddy_free_count,0U);
        {
            kernel::string ks;
            tmock::assert_equiv(buddy_alloc_count,0U);
            tmock::assert_equiv(buddy_free_count,0U);
            tmock::assert_equiv(ks.strlen(),0U);
            tmock::assert_equiv(ks.avail(),0U);

            ks.printf("Hello, world!");
            tmock::assert_equiv(buddy_alloc_count,1U);

            ks.printf("bye bye");
            tmock::assert_equiv(ks,"Hello, world!bye bye");
            tmock::assert_equiv(ks.len,4096U);
        }
        tmock::assert_equiv(buddy_alloc_count,1U);
        tmock::assert_equiv(buddy_free_count,1U);
    }

    TMOCK_TEST(test_buddy_string_fmt_constructor)
    {
        std::string ss;
        for (size_t i=0; i<256; ++i)
            ss += "12345678901234567890123456789012";
        tmock::assert_equiv(ss.size(),8192U);

        kernel::string ks("%s",ss.c_str());
        tmock::assert_equiv(ks.strlen(),8192U);
        tmock::assert_equiv(ks.avail(),8191U);
        tmock::assert_equiv(ks.len,16384U);
    }

    TMOCK_TEST(test_kstring_grows)
    {
        tmock::assert_equiv(buddy_alloc_count,0U);
        tmock::assert_equiv(buddy_free_count,0U);
        {
            kernel::string ks;
            std::string ss;

            tmock::assert_equiv(buddy_alloc_count,0U);
            tmock::assert_equiv(buddy_free_count,0U);

            for (size_t i=0; i<127; ++i)
            {
                ks.printf("12345678901234567890123456789012");
                ss += "12345678901234567890123456789012";
            }
            tmock::assert_equiv(buddy_alloc_count,1U);
            tmock::assert_equiv(ks.strlen(),4064U);
            tmock::assert_equiv(ks.avail(),31U);
            tmock::assert_equiv(ks.len,4096U);

            ks.printf("1234567890123456789012345678901");
            ss += "1234567890123456789012345678901";
            tmock::assert_equiv(ks.strlen(),4095U);
            tmock::assert_equiv(ks.avail(),0U);
            tmock::assert_equiv(ks.len,4096U);

            tmock::assert_equiv(buddy_alloc_count,1U);
            tmock::assert_equiv(buddy_free_count,0U);

            ks.printf("Hello!");
            ss += "Hello!";

            tmock::assert_equiv(buddy_alloc_count,2U);
            tmock::assert_equiv(buddy_free_count,1U);
            tmock::assert_equiv(ks.strlen(),4101U);
            tmock::assert_equiv(ks.avail(),4090U);
            tmock::assert_equiv(ks.len,8192U);

            tmock::assert_equiv(ss.c_str(),ks);
        }
        tmock::assert_equiv(buddy_alloc_count,2U);
        tmock::assert_equiv(buddy_free_count,2U);
    }

    TMOCK_TEST(test_kstring_append)
    {
        tmock::assert_equiv(buddy_alloc_count,0U);
        tmock::assert_equiv(buddy_free_count,0U);
        {
            kernel::string ks;
            std::string ss;

            tmock::assert_equiv(buddy_alloc_count,0U);
            tmock::assert_equiv(buddy_free_count,0U);

            for (size_t i=0; i<127; ++i)
            {
                ks += "12345678901234567890123456789012";
                ss += "12345678901234567890123456789012";
            }
            tmock::assert_equiv(buddy_alloc_count,1U);
            tmock::assert_equiv(ks.strlen(),4064U);
            tmock::assert_equiv(ks.avail(),31U);
            tmock::assert_equiv(ks.len,4096U);

            ks += "1234567890123456789012345678901";
            ss += "1234567890123456789012345678901";
            tmock::assert_equiv(ks.strlen(),4095U);
            tmock::assert_equiv(ks.avail(),0U);
            tmock::assert_equiv(ks.len,4096U);

            tmock::assert_equiv(buddy_alloc_count,1U);
            tmock::assert_equiv(buddy_free_count,0U);

            ks += "Hello!";
            ss += "Hello!";

            tmock::assert_equiv(buddy_alloc_count,2U);
            tmock::assert_equiv(buddy_free_count,1U);
            tmock::assert_equiv(ks.strlen(),4101U);
            tmock::assert_equiv(ks.avail(),4090U);
            tmock::assert_equiv(ks.len,8192U);

            tmock::assert_equiv(ss.c_str(),ks);

            ks += ss.c_str();
            ss += ss;

            tmock::assert_equiv(buddy_alloc_count,3U);
            tmock::assert_equiv(buddy_free_count,2U);
            tmock::assert_equiv(ks.strlen(),8202U);
            tmock::assert_equiv(ks.avail(),8181U);
            tmock::assert_equiv(ks.len,16384U);

            tmock::assert_equiv(ss.c_str(),ks);
        }
        tmock::assert_equiv(buddy_alloc_count,3U);
        tmock::assert_equiv(buddy_free_count,3U);
    }

    TMOCK_TEST(test_kstring_ends_with)
    {
        kernel::string ks;
        TASSERT(ks.ends_with(""));
        TASSERT(!ks.ends_with("hello"));

        ks.printf("hell");
        TASSERT(ks.ends_with(""));
        TASSERT(!ks.ends_with("hello"));

        ks.printf("o");
        TASSERT(ks.ends_with(""));
        TASSERT(ks.ends_with("hello"));

        ks.printf("!");
        TASSERT(ks.ends_with(""));
        TASSERT(!ks.ends_with("hello"));
    }
};

TMOCK_MAIN();
