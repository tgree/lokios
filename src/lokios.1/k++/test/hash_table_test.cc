#include "../hash_table.h"
#include "net/ip/ip.h"
#include "net/eth/addr.h"
#include "tmock/tmock.h"
#include <vector>
#include <algorithm>
#include <unordered_map>

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
        hash::table<int,int,one_hasher<int>> t;
        t.emplace(1,1);
        t.emplace(2,2);
        t.emplace(3,3);
        tmock::assert_equiv(t.bins[1].size(),3U);
    }

    TMOCK_TEST(test_unlink)
    {
        hash::table<int,elem> t;
        elem& v = t.emplace(123,4,5,6);
        tmock::assert_equiv(constructions,1U);
        tmock::assert_equiv(destructions,0U);
        tmock::assert_equiv(t.size(),1U);
        TASSERT(t.contains(123));
        t.unlink_value(&v);
        tmock::assert_equiv(t.size(),1U);
        TASSERT(!t.contains(123));
        tmock::assert_equiv(constructions,1U);
        tmock::assert_equiv(destructions,0U);
        tmock::assert_equiv(&klist_front(t.unlinked_nodes,link)->v,&v);
        t.erase_value(&v);
        TASSERT(t.empty());
        TASSERT(!t.contains(123));
        TASSERT(t.unlinked_nodes.empty());
        tmock::assert_equiv(constructions,1U);
        tmock::assert_equiv(destructions,1U);
    }

    TMOCK_TEST(test_grow_shrink)
    {
        hash::table<int,int> t;
        std::vector<int> elems;
        tmock::assert_equiv(t.nbins,256U);
        for (int i=0; i<6*256-1; ++i)
        {
            t.emplace(i,i);
            elems.push_back(i);
        }
        tmock::assert_equiv(t.nbins,256U);
        for (size_t i=0; i<t.nbins; ++i)
        {
            size_t bin_size = t.bins[i].size();
            TASSERT(bin_size == 5 || bin_size == 6);
        }
        t.emplace(6*256-1,6*256-1);
        elems.push_back(6*256-1);
        tmock::assert_equiv(t.nbins,512U);
        tmock::assert_equiv(t.size(),6*256U);
        for (int i=0; i<6*256; ++i)
            tmock::assert_equiv(t[i],i);
        for (size_t i=0; i<t.nbins; ++i)
        {
            size_t bin_size = t.bins[i].size();
            TASSERT(bin_size == 3);
        }

        random_shuffle(elems.begin(),elems.end());
        for (int i=0; i<2*256; ++i)
            t.erase(elems[i]);
        tmock::assert_equiv(t.size(),4*256U);
        tmock::assert_equiv(t.nbins,512U);

        t.erase(elems[2*256]);
        tmock::assert_equiv(t.size(),4*256U-1U);
        tmock::assert_equiv(t.nbins,256U);

        for (int i=2*256+1; i<6*256; ++i)
            t.erase(elems[i]);
        TASSERT(t.empty());
        tmock::assert_equiv(t.size(),0U);
        tmock::assert_equiv(t.nbins,256U);
    }

    TMOCK_TEST(test_clear)
    {
        hash::table<int,int> t;

        TASSERT(t.empty());
        tmock::assert_equiv(t.size(),0U);
        for (size_t i=0; i<t.nbins; ++i)
            TASSERT(t.bins[i].empty());

        for (size_t i=0; i<t.nbins; ++i)
            t.emplace(i,i);
        TASSERT(!t.empty());
        tmock::assert_equiv(t.size(),t.nbins);
        for (size_t i=0; i<t.nbins; ++i)
            TASSERT(!t.bins[i].empty());

        t.clear();
        TASSERT(t.empty());
        tmock::assert_equiv(t.size(),0U);
        for (size_t i=0; i<t.nbins; ++i)
            TASSERT(t.bins[i].empty());
    }

    TMOCK_TEST(test_iterator_empty)
    {
        hash::table<const char*,int> ht;
        size_t count = 0;
        for (auto& e __UNUSED__ : ht)
            ++count;
        tmock::assert_equiv(count,0U);
    }

    struct iterator_test_elem
    {
        const char* k;
        int         v;
    };
    TMOCK_TEST(test_iterator_many)
    {
        iterator_test_elem elems[] = {
            {"one",  1},
            {"two",  2},
            {"three",3},
            {"four", 4},
            {"five", 5},
            {"six",  6},
            {"seven",7},
            {"eight",8},
            {"nine", 9},
            {"ten",  10},
        };
        std::unordered_map<std::string,int> um;
        hash::table<const char*,int> ht;
        for (auto& e : elems)
        {
            um.emplace(e.k,e.v);
            ht.emplace(e.k,e.v);
        }
        for (auto& e : ht)
        {
            TASSERT(um.count(e.k));
            um.erase(e.k);
        }
        TASSERT(um.empty());
    }
};

TMOCK_MAIN();
