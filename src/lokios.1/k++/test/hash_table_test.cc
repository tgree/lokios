#include "../hash_table.h"
#include "net/ip/ip.h"
#include "net/eth/addr.h"
#include "tmock/tmock.h"

static size_t constructions;
static size_t destructions;
struct elem
{
    unsigned int    a;
    char            b;
    long            c;

    elem(unsigned int a, char b, long c):a(a),b(b),c(c)
    {
        ++constructions;
    }
    ~elem()
    {
        ++destructions;
    }
};
static inline bool operator==(const elem& lhs, const elem& rhs)
{
    return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c;
}

template<typename T>
static inline size_t one_hasher(const T& k)
{
    return 1;
}

class tmock_test
{
    TMOCK_TEST(test_construct_destroy)
    {
        hash::table<int,int> t;
        tmock::assert_equiv(t.nbins,256U);
    }

    TMOCK_TEST(test_string_hash_table)
    {
        hash::table<const char*,int> t;
        t.insert("one",1);
        t.insert("two",2);
        t.insert("three",3);

        char one[] = {'o','n','e','\0'};
        TASSERT(one != (char*)"one");
        tmock::assert_equiv(t[one],1);
        tmock::assert_equiv(t["two"],2);
        tmock::assert_equiv(t["three"],3);

        auto* p1 = &t["three"];
        t["three"] = 4;
        tmock::assert_equiv(t["three"],4);
        tmock::assert_equiv(&t["three"],p1);

        TASSERT(!t.contains("thre"));
        TASSERT(!t.contains(""));

        try
        {
            t["blah"] = 1;
            tmock::abort("should have thrown exception");
        }
        catch (hash::no_such_key_exception&)
        {
        }
    }

    TMOCK_TEST(test_arp_table)
    {
        hash::table<ipv4::addr,eth::addr> t;
        t.insert(ipv4::addr{1,2,3,4},eth::addr{1,2,3,4,0,0});
        t.insert(ipv4::addr{2,2,3,4},eth::addr{1,2,3,4,0,1});
        t.insert(ipv4::addr{1,2,3,5},eth::addr{1,2,3,4,0,2});

        tmock::assert_equiv(t[ipv4::addr{1,2,3,4}],eth::addr{1,2,3,4,0,0});
        tmock::assert_equiv(t[ipv4::addr{2,2,3,4}],eth::addr{1,2,3,4,0,1});
        tmock::assert_equiv(t[ipv4::addr{1,2,3,5}],eth::addr{1,2,3,4,0,2});
    }

    TMOCK_TEST(test_inplace_structs)
    {
        hash::table<size_t,elem> t;

        TASSERT(t.empty());

        t.emplace(10,1,2,3);
        TASSERT(!t.empty());
        tmock::assert_equiv(t.size(),1U);
        tmock::assert_equiv(constructions,1U);
        tmock::assert_equiv(destructions,0U);

        t.emplace(20,4,5,6);
        TASSERT(!t.empty());
        tmock::assert_equiv(t.size(),2U);
        tmock::assert_equiv(constructions,2U);
        tmock::assert_equiv(destructions,0U);

        t.emplace(-11,2,2,2);
        TASSERT(!t.empty());
        tmock::assert_equiv(t.size(),3U);
        tmock::assert_equiv(constructions,3U);
        tmock::assert_equiv(destructions,0U);

        tmock::assert_equiv(t[ 10],elem(1,2,3));
        tmock::assert_equiv(t[ 20],elem(4,5,6));
        tmock::assert_equiv(t[-11],elem(2,2,2));
        tmock::assert_equiv(constructions,6U);
        tmock::assert_equiv(destructions,3U);

        auto& e = t[20];
        t.erase_value(&e);
        TASSERT(!t.empty());
        tmock::assert_equiv(t.size(),2U);
        tmock::assert_equiv(constructions,6U);
        tmock::assert_equiv(destructions,4U);
        TASSERT(!t.contains(20));

        t.erase(-11);
        TASSERT(!t.empty());
        tmock::assert_equiv(t.size(),1U);
        tmock::assert_equiv(constructions,6U);
        tmock::assert_equiv(destructions,5U);
        TASSERT(!t.contains(-11));

        t.erase(10);
        TASSERT(t.empty());
        tmock::assert_equiv(t.size(),0U);
        tmock::assert_equiv(constructions,6U);
        tmock::assert_equiv(destructions,6U);
        TASSERT(!t.contains(10));
    }

    TMOCK_TEST(test_custom_hasher)
    {
        hash::table<int,int,one_hasher> t;
        t.emplace(1,1);
        t.emplace(2,2);
        t.emplace(3,3);
        tmock::assert_equiv(t.bins[1].size(),3U);
    }
};

TMOCK_MAIN();
