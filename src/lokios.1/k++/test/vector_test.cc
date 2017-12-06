#include "../vector.h"
#include "tmock/tmock.h"
#include <string.h>
#include <set>

struct entry
{
    int a;
    int b;
    const char* c;
    bool d;

    static size_t destructor_calls;

    entry(entry&& other):
        a(other.a),b(other.b),c(other.c),d(other.d)
    {
        other.c = NULL;
    }
    entry(const entry& other):a(other.a),b(other.b),c(other.c),d(other.d) {}
    constexpr entry(int a, int b, const char* c):a(a),b(b),c(c),d(false) {}
    inline ~entry() {kernel::kassert(!d); d = true; ++destructor_calls;}
};

size_t entry::destructor_calls = 0;

struct default_constructible_entry : public entry
{
    constexpr default_constructible_entry():entry(1,2,"three") {}
};

void
kernel::panic(const char* s) noexcept
{
    tmock::abort(s);
}

void*
kernel::page_alloc()
{
    void* p;
    kernel::kassert(posix_memalign(&p,PAGE_SIZE,PAGE_SIZE) == 0);
    return p;
}

void
kernel::page_free(void* p)
{
    free(p);
}

class tmock_test
{
    TMOCK_TEST(test_subscript_operator)
    {
        kernel::vector<entry> v;
        entry entries[] = {
            entry(1,2,"one two"),
            entry(3,4,"three four"),
            entry(5,6,"five six"),
            entry(0,0,"the end"),
        };
        for (const auto& e : entries)
            v.push_back(e);
        kernel::kassert(v.size() == kernel::nelems(entries));
        for (size_t i=0; i<v.size(); ++i)
        {
            kernel::kassert(v[i].a == entries[i].a);
            kernel::kassert(v[i].b == entries[i].b);
            kernel::kassert(v[i].c == entries[i].c);
        }
    }

    TMOCK_TEST_EXPECT_FAILURE(test_subscript_operator_bounds_check_empty_vector)
    {
        kernel::vector<entry> v;
        v[0].a = 0;
    }

    TMOCK_TEST_EXPECT_FAILURE(test_subscript_operator_bounds_check)
    {
        kernel::vector<entry> v;
        v.emplace_back(1,2,"one two");
        v.emplace_back(3,4,"three four");
        v.emplace_back(5,6,"five six");
        v.emplace_back(0,0,"the end");
        v[4].a = 0;
    }

    TMOCK_TEST(test_push_back_copy)
    {
        {
            kernel::vector<entry> v;
            {
                entry entry0(1,2,"one two");
                entry entry1(3,4,"three four");
                entry entry2(5,6,"five six");
                entry entry3(0,0,"the end");

                v.push_back(entry0);
                v.push_back(entry1);
                v.push_back(entry2);
                v.push_back(entry3);
            }
            kernel::kassert(entry::destructor_calls == 4);

            kernel::kassert(v.size() == 4);
            kernel::kassert(v[0].a == 1 && v[0].b == 2 &&
                            !strcmp(v[0].c,"one two"));
            kernel::kassert(v[1].a == 3 && v[1].b == 4 &&
                            !strcmp(v[1].c,"three four"));
            kernel::kassert(v[2].a == 5 && v[2].b == 6 &&
                            !strcmp(v[2].c,"five six"));
            kernel::kassert(v[3].a == 0 && v[3].b == 0 &&
                            !strcmp(v[3].c,"the end"));
        }

        kernel::kassert(entry::destructor_calls == 8);
    }

    TMOCK_TEST(test_push_back_move)
    {
        {
            kernel::vector<entry> v;
            entry entry0(1,2,"one two");
            entry entry1(3,4,"three four");
            entry entry2(5,6,"five six");
            entry entry3(0,0,"the end");

            v.push_back(std::move(entry0));
            v.push_back(std::move(entry1));
            v.push_back(std::move(entry2));
            v.push_back(std::move(entry3));

            kernel::kassert(entry0.c == NULL);
            kernel::kassert(entry1.c == NULL);
            kernel::kassert(entry2.c == NULL);
            kernel::kassert(entry3.c == NULL);

            kernel::kassert(v.size() == 4);
            kernel::kassert(v[0].a == 1 && v[0].b == 2 &&
                            !strcmp(v[0].c,"one two"));
            kernel::kassert(v[1].a == 3 && v[1].b == 4 &&
                            !strcmp(v[1].c,"three four"));
            kernel::kassert(v[2].a == 5 && v[2].b == 6 &&
                            !strcmp(v[2].c,"five six"));
            kernel::kassert(v[3].a == 0 && v[3].b == 0 &&
                            !strcmp(v[3].c,"the end"));
        }

        kernel::kassert(entry::destructor_calls == 8);
    }

    TMOCK_TEST(test_emplace_back)
    {
        {
            kernel::vector<entry> v;
            v.emplace_back(1,2,"one two");
            v.emplace_back(3,4,"three four");
            v.emplace_back(5,6,"five six");
            v.emplace_back(0,0,"the end");
            kernel::kassert(entry::destructor_calls == 0);

            kernel::kassert(v.size() == 4);
            kernel::kassert(v[0].a == 1 && v[0].b == 2 &&
                            !strcmp(v[0].c,"one two"));
            kernel::kassert(v[1].a == 3 && v[1].b == 4 &&
                            !strcmp(v[1].c,"three four"));
            kernel::kassert(v[2].a == 5 && v[2].b == 6 &&
                            !strcmp(v[2].c,"five six"));
            kernel::kassert(v[3].a == 0 && v[3].b == 0 &&
                            !strcmp(v[3].c,"the end"));
        }

        kernel::kassert(entry::destructor_calls == 4);
    }

    TMOCK_TEST(test_empty_vector_with_required_constructor_args)
    {
        {
            // Even though the entry type requires constructor arguments, we
            // should still be able to make an empty vector of these.
            kernel::vector<entry> v;
        }

        kernel::kassert(entry::destructor_calls == 0);
    }

    TMOCK_TEST(test_nonempty_vector_constructor)
    {
        {
            kernel::vector<default_constructible_entry> v(10);
            kernel::kassert(v.size() == 10);
            size_t i = 0;
            for (const auto& e : v)
            {
                ++i;
                kernel::kassert(e.a == 1 && e.b == 2 && !strcmp(e.c,"three"));
            }
            kernel::kassert(i == 10);
        }

        kernel::kassert(entry::destructor_calls == 10);
    }

    TMOCK_TEST(test_move_vector_constructor)
    {
        {
            kernel::vector<default_constructible_entry> v1(7);
            kernel::vector<default_constructible_entry> v2(std::move(v1));

            kernel::kassert(v1.size() == 0);
            kernel::kassert(v1.begin() == v1.end());
            kernel::kassert(v2.size() == 7);
            kernel::kassert(v2.begin() != v2.end());
            kernel::kassert(v1.begin() != v2.begin());
            kernel::kassert(v1.end() != v2.end());

            size_t i = 0;
            for (const auto& e __attribute__((unused)) : v1)
                ++i;
            for (const auto& e : v2)
            {
                ++i;
                kernel::kassert(e.a == 1 && e.b == 2 && !strcmp(e.c,"three"));
            }
            kernel::kassert(i == 7);
        }

        kernel::kassert(entry::destructor_calls == 7);
    }

    TMOCK_TEST(test_copy_vector_constructor)
    {
        {
            kernel::vector<entry> v1;
            v1.emplace_back(1,2,"one two");
            v1.emplace_back(3,4,"three four");
            v1.emplace_back(5,6,"five six");
            v1.emplace_back(0,0,"the end");

            kernel::vector<entry> v2(v1);
            kernel::kassert(v2.size() == 4);

            std::set<entry*> entries;
            for (auto& e : v1)
                entries.insert(&e);
            for (auto& e : v2)
                entries.insert(&e);
            kernel::kassert(entries.size() == 8);
        }

        kernel::kassert(entry::destructor_calls == 8);
    }

    TMOCK_TEST(test_fill_page)
    {
        kernel::vector<entry> v;
        kernel::kassert(v.capacity() == PAGE_SIZE/sizeof(entry));
        for (size_t i=0; i<v.capacity(); ++i)
            v.emplace_back(i,i,"asdf");
    }

    TMOCK_TEST_EXPECT_FAILURE(test_overflow_page)
    {
        kernel::vector<entry> v;
        kernel::kassert(v.capacity() == PAGE_SIZE/sizeof(entry));
        for (size_t i=0; i<v.capacity()+1; ++i)
            v.emplace_back(i,i,"asdf");
    }

    TMOCK_TEST(test_emplace_at_begin)
    {
        kernel::vector<entry> v;
        v.emplace_back(1,2,"one two");
        v.emplace_back(3,4,"three four");
        v.emplace_back(5,6,"five six");
        v.emplace_back(0,0,"the end");

        v.emplace(v.begin(),9,9,"surprise!");
        kernel::kassert(v.size() == 5);
        kernel::kassert(v[0].a == 9 && v[0].b == 9 && !strcmp(v[0].c,"surprise!"));
        kernel::kassert(v[1].a == 1 && v[1].b == 2 && !strcmp(v[1].c,"one two"));
        kernel::kassert(v[2].a == 3 && v[2].b == 4 && !strcmp(v[2].c,"three four"));
        kernel::kassert(v[3].a == 5 && v[3].b == 6 && !strcmp(v[3].c,"five six"));
        kernel::kassert(v[4].a == 0 && v[4].b == 0 && !strcmp(v[4].c,"the end"));
        kernel::kassert(entry::destructor_calls == 4);
    }

    TMOCK_TEST(test_emplace_at_end)
    {
        kernel::vector<entry> v;
        v.emplace_back(1,2,"one two");
        v.emplace_back(3,4,"three four");
        v.emplace_back(5,6,"five six");
        v.emplace_back(0,0,"the end");

        v.emplace(v.end(),9,9,"surprise!");
        kernel::kassert(v.size() == 5);
        kernel::kassert(v[0].a == 1 && v[0].b == 2 && !strcmp(v[0].c,"one two"));
        kernel::kassert(v[1].a == 3 && v[1].b == 4 && !strcmp(v[1].c,"three four"));
        kernel::kassert(v[2].a == 5 && v[2].b == 6 && !strcmp(v[2].c,"five six"));
        kernel::kassert(v[3].a == 0 && v[3].b == 0 && !strcmp(v[3].c,"the end"));
        kernel::kassert(v[4].a == 9 && v[4].b == 9 && !strcmp(v[4].c,"surprise!"));
        kernel::kassert(entry::destructor_calls == 0);
    }

    TMOCK_TEST(test_emplace_in_middle)
    {
        kernel::vector<entry> v;
        v.emplace_back(1,2,"one two");
        v.emplace_back(3,4,"three four");
        v.emplace_back(5,6,"five six");
        v.emplace_back(0,0,"the end");

        auto i = v.begin();
        ++i;
        ++i;
        v.emplace(i,9,9,"surprise!");
        kernel::kassert(v.size() == 5);
        kernel::kassert(v[0].a == 1 && v[0].b == 2 && !strcmp(v[0].c,"one two"));
        kernel::kassert(v[1].a == 3 && v[1].b == 4 && !strcmp(v[1].c,"three four"));
        kernel::kassert(v[2].a == 9 && v[2].b == 9 && !strcmp(v[2].c,"surprise!"));
        kernel::kassert(v[3].a == 5 && v[3].b == 6 && !strcmp(v[3].c,"five six"));
        kernel::kassert(v[4].a == 0 && v[4].b == 0 && !strcmp(v[4].c,"the end"));
        kernel::kassert(entry::destructor_calls == 2);
    }

    TMOCK_TEST(test_emplace_empty_begin)
    {
        kernel::vector<entry> v;
        v.emplace(v.begin(),9,9,"surprise!");
        kernel::kassert(v.size() == 1);
        kernel::kassert(v[0].a == 9 && v[0].b == 9 && !strcmp(v[0].c,"surprise!"));
        kernel::kassert(entry::destructor_calls == 0);
    }

    TMOCK_TEST(test_emplace_empty_end)
    {
        kernel::vector<entry> v;
        v.emplace(v.end(),9,9,"surprise!");
        kernel::kassert(v.size() == 1);
        kernel::kassert(v[0].a == 9 && v[0].b == 9 && !strcmp(v[0].c,"surprise!"));
        kernel::kassert(entry::destructor_calls == 0);
    }

    TMOCK_TEST(test_insert_at_begin)
    {
        kernel::vector<entry> v;
        v.emplace_back(1,2,"one two");
        v.emplace_back(3,4,"three four");
        v.emplace_back(5,6,"five six");
        v.emplace_back(0,0,"the end");

        v.insert(v.begin(),entry(9,9,"surprise!"));
        kernel::kassert(v.size() == 5);
        kernel::kassert(v[0].a == 9 && v[0].b == 9 && !strcmp(v[0].c,"surprise!"));
        kernel::kassert(v[1].a == 1 && v[1].b == 2 && !strcmp(v[1].c,"one two"));
        kernel::kassert(v[2].a == 3 && v[2].b == 4 && !strcmp(v[2].c,"three four"));
        kernel::kassert(v[3].a == 5 && v[3].b == 6 && !strcmp(v[3].c,"five six"));
        kernel::kassert(v[4].a == 0 && v[4].b == 0 && !strcmp(v[4].c,"the end"));
        kernel::kassert(entry::destructor_calls == 4 + 1);
    }

    TMOCK_TEST(test_insert_at_end)
    {
        kernel::vector<entry> v;
        v.emplace_back(1,2,"one two");
        v.emplace_back(3,4,"three four");
        v.emplace_back(5,6,"five six");
        v.emplace_back(0,0,"the end");

        v.insert(v.end(),entry(9,9,"surprise!"));
        kernel::kassert(v.size() == 5);
        kernel::kassert(v[0].a == 1 && v[0].b == 2 && !strcmp(v[0].c,"one two"));
        kernel::kassert(v[1].a == 3 && v[1].b == 4 && !strcmp(v[1].c,"three four"));
        kernel::kassert(v[2].a == 5 && v[2].b == 6 && !strcmp(v[2].c,"five six"));
        kernel::kassert(v[3].a == 0 && v[3].b == 0 && !strcmp(v[3].c,"the end"));
        kernel::kassert(v[4].a == 9 && v[4].b == 9 && !strcmp(v[4].c,"surprise!"));
        kernel::kassert(entry::destructor_calls == 0 + 1);
    }

    TMOCK_TEST(test_insert_in_middle)
    {
        kernel::vector<entry> v;
        v.emplace_back(1,2,"one two");
        v.emplace_back(3,4,"three four");
        v.emplace_back(5,6,"five six");
        v.emplace_back(0,0,"the end");

        auto i = v.begin();
        ++i;
        ++i;
        v.insert(i,entry(9,9,"surprise!"));
        kernel::kassert(v.size() == 5);
        kernel::kassert(v[0].a == 1 && v[0].b == 2 && !strcmp(v[0].c,"one two"));
        kernel::kassert(v[1].a == 3 && v[1].b == 4 && !strcmp(v[1].c,"three four"));
        kernel::kassert(v[2].a == 9 && v[2].b == 9 && !strcmp(v[2].c,"surprise!"));
        kernel::kassert(v[3].a == 5 && v[3].b == 6 && !strcmp(v[3].c,"five six"));
        kernel::kassert(v[4].a == 0 && v[4].b == 0 && !strcmp(v[4].c,"the end"));
        kernel::kassert(entry::destructor_calls == 2 + 1);
    }

    TMOCK_TEST(test_insert_empty_begin)
    {
        kernel::vector<entry> v;
        v.insert(v.begin(),entry(9,9,"surprise!"));
        kernel::kassert(v.size() == 1);
        kernel::kassert(v[0].a == 9 && v[0].b == 9 && !strcmp(v[0].c,"surprise!"));
        kernel::kassert(entry::destructor_calls == 0 + 1);
    }

    TMOCK_TEST(test_insert_empty_end)
    {
        kernel::vector<entry> v;
        v.insert(v.end(),entry(9,9,"surprise!"));
        kernel::kassert(v.size() == 1);
        kernel::kassert(v[0].a == 9 && v[0].b == 9 && !strcmp(v[0].c,"surprise!"));
        kernel::kassert(entry::destructor_calls == 0 + 1);
    }

    TMOCK_TEST(test_erase_begin)
    {
        kernel::vector<entry> v;
        v.emplace_back(1,2,"one two");
        v.emplace_back(3,4,"three four");
        v.emplace_back(5,6,"five six");
        v.emplace_back(0,0,"the end");

        v.erase(v.begin());
        kernel::kassert(v.size() == 3);
        kernel::kassert(v[0].a == 3 && v[0].b == 4 && !strcmp(v[0].c,"three four"));
        kernel::kassert(v[1].a == 5 && v[1].b == 6 && !strcmp(v[1].c,"five six"));
        kernel::kassert(v[2].a == 0 && v[2].b == 0 && !strcmp(v[2].c,"the end"));
        kernel::kassert(entry::destructor_calls == 4);
    }

    TMOCK_TEST(test_erase_end)
    {
        kernel::vector<entry> v;
        v.emplace_back(1,2,"one two");
        v.emplace_back(3,4,"three four");
        v.emplace_back(5,6,"five six");
        v.emplace_back(0,0,"the end");

        v.erase(v.end() - 1);
        kernel::kassert(v.size() == 3);
        kernel::kassert(v[0].a == 1 && v[0].b == 2 && !strcmp(v[0].c,"one two"));
        kernel::kassert(v[1].a == 3 && v[1].b == 4 && !strcmp(v[1].c,"three four"));
        kernel::kassert(v[2].a == 5 && v[2].b == 6 && !strcmp(v[2].c,"five six"));
        kernel::kassert(entry::destructor_calls == 1);
    }

    TMOCK_TEST(test_erase_middle)
    {
        kernel::vector<entry> v;
        v.emplace_back(1,2,"one two");
        v.emplace_back(3,4,"three four");
        v.emplace_back(5,6,"five six");
        v.emplace_back(0,0,"the end");

        auto i = v.begin();
        ++i;
        ++i;
        v.erase(i);
        kernel::kassert(v.size() == 3);
        kernel::kassert(v[0].a == 1 && v[0].b == 2 && !strcmp(v[0].c,"one two"));
        kernel::kassert(v[1].a == 3 && v[1].b == 4 && !strcmp(v[1].c,"three four"));
        kernel::kassert(v[2].a == 0 && v[2].b == 0 && !strcmp(v[2].c,"the end"));
        kernel::kassert(entry::destructor_calls == 2);
    }
};

TMOCK_MAIN();
