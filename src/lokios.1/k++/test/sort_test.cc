#include "../sort.h"
#include "../klist.h"
#include "hdr/types.h"
#include "kern/kassert.h"
#include "tmock/tmock.h"
#include <string.h>
#include <vector>

using kernel::_kassert;

static uint64_t unsorted[] = {
    5348143246242264759U,  5177841320952229490U,  18314874294901963553U,
    18329021337179044916U, 2863493462733046394U,  1474179452908045113U,
    9728507484038684402U,  15297565252741334801U, 7172603433793638633U,
    6137543974106712231U,  2606012939359927532U,  6697281542789566294U,
    14136123427750789692U, 1889866177667698485U,  13899755160123547546U,
    158473854127908925U,   3255686866180357042U,  7363693453610430483U,
    5995941077254774310U,  13172073729271831619U, 2816454185709291154U,
    15821989847356972469U, 3667673352289628414U,  2406097274458645962U,
    16953336158990332275U, 1508413926762475764U,  12761863881795824819U,
    2228637649612286440U,  18021793853434783484U, 12055299697284679495U,
    3701640372879985040U,  9800223693216787743U,  820503570934051031U,
    17273654005766225673U, 5187930867383476423U,  7391241868589089571U,
    5022510572958168610U,  2680258536359251222U,  15096542797592697308U,
    7487547825113128702U,  15874581981109060505U, 16955513889609682288U,
    10331321927948507354U, 12221264090115876043U, 17724408532758296180U,
    11969551497158194717U, 12686243977031950924U, 10108652403002574225U,
    13977965880698852745U, 9764660563277502136U,  4585679009838385808U,
    2438313951837164819U,  2690875123227085195U,  8858734852717700592U,
    14755993554085194414U, 15876873306731207704U, 4322782588150942200U,
    12908668676219322026U, 8249507356242249810U,  6242879175861420454U,
    3590856021272482590U,  10529505104718202895U, 6562556860929724735U,
    3519886655119207331U,  9024308103385636902U,  9119475711907595461U,
    628723085193055972U,   10023228734451811515U, 17297720569164861532U,
    14762254659146613901U, 9649342976882148355U,  4071035467551236755U,
    8071775796420828616U,  12564904614560838178U, 17990232158220747954U,
    10098212089517543403U, 17908441716727932584U, 12089549622339388482U,
    3745762795569650378U,  6636335561763662944U,  12811698310399060959U,
    12146683302820224830U, 17978599480276005105U, 8596506319685128236U,
    3062818833589731897U,  4151412767871280937U,  6409014862883956040U,
    15449263102422324838U, 1424041189634504844U,  5655632857543853663U,
    14434904205800204499U, 13335285705790475522U, 12915684116696778907U,
    1117003441072254254U,  14376826713242518068U, 18059244139052326149U,
    2341949525730744502U,  4725262279963623277U,  5198426872830050498U,
    5076940979161609199U,  14896174112748267235U, 6100227874920312209U,
    6086071843404710944U,  16787610829667187851U, 396451834957816679U,
    9961394205039295557U,  3578013027031720940U,  8593945589366738097U,
    2952721407353647936U,  6491644829783454222U,  4285000448586299231U,
    17952483584172847999U, 3001994775718695469U,  7895840326011240686U,
    12951849359810727491U, 544641389391780892U,   7648994145879941123U,
    7406752259977446583U,  4204242734194395650U,  7499891805773168115U,
    5300300986769881057U,  5945360659621610973U,  17886580996153038664U,
    337877981927391222U,   15579802547403153288U, 5262260931170981968U,
    8221671206608844148U,  3645357088317219939U,
};

static uint64_t sorted[] = {
    158473854127908925U,   337877981927391222U,   396451834957816679U,
    544641389391780892U,   628723085193055972U,   820503570934051031U,
    1117003441072254254U,  1424041189634504844U,  1474179452908045113U,
    1508413926762475764U,  1889866177667698485U,  2228637649612286440U,
    2341949525730744502U,  2406097274458645962U,  2438313951837164819U,
    2606012939359927532U,  2680258536359251222U,  2690875123227085195U,
    2816454185709291154U,  2863493462733046394U,  2952721407353647936U,
    3001994775718695469U,  3062818833589731897U,  3255686866180357042U,
    3519886655119207331U,  3578013027031720940U,  3590856021272482590U,
    3645357088317219939U,  3667673352289628414U,  3701640372879985040U,
    3745762795569650378U,  4071035467551236755U,  4151412767871280937U,
    4204242734194395650U,  4285000448586299231U,  4322782588150942200U,
    4585679009838385808U,  4725262279963623277U,  5022510572958168610U,
    5076940979161609199U,  5177841320952229490U,  5187930867383476423U,
    5198426872830050498U,  5262260931170981968U,  5300300986769881057U,
    5348143246242264759U,  5655632857543853663U,  5945360659621610973U,
    5995941077254774310U,  6086071843404710944U,  6100227874920312209U,
    6137543974106712231U,  6242879175861420454U,  6409014862883956040U,
    6491644829783454222U,  6562556860929724735U,  6636335561763662944U,
    6697281542789566294U,  7172603433793638633U,  7363693453610430483U,
    7391241868589089571U,  7406752259977446583U,  7487547825113128702U,
    7499891805773168115U,  7648994145879941123U,  7895840326011240686U,
    8071775796420828616U,  8221671206608844148U,  8249507356242249810U,
    8593945589366738097U,  8596506319685128236U,  8858734852717700592U,
    9024308103385636902U,  9119475711907595461U,  9649342976882148355U,
    9728507484038684402U,  9764660563277502136U,  9800223693216787743U,
    9961394205039295557U,  10023228734451811515U, 10098212089517543403U,
    10108652403002574225U, 10331321927948507354U, 10529505104718202895U,
    11969551497158194717U, 12055299697284679495U, 12089549622339388482U,
    12146683302820224830U, 12221264090115876043U, 12564904614560838178U,
    12686243977031950924U, 12761863881795824819U, 12811698310399060959U,
    12908668676219322026U, 12915684116696778907U, 12951849359810727491U,
    13172073729271831619U, 13335285705790475522U, 13899755160123547546U,
    13977965880698852745U, 14136123427750789692U, 14376826713242518068U,
    14434904205800204499U, 14755993554085194414U, 14762254659146613901U,
    14896174112748267235U, 15096542797592697308U, 15297565252741334801U,
    15449263102422324838U, 15579802547403153288U, 15821989847356972469U,
    15874581981109060505U, 15876873306731207704U, 16787610829667187851U,
    16953336158990332275U, 16955513889609682288U, 17273654005766225673U,
    17297720569164861532U, 17724408532758296180U, 17886580996153038664U,
    17908441716727932584U, 17952483584172847999U, 17978599480276005105U,
    17990232158220747954U, 18021793853434783484U, 18059244139052326149U,
    18314874294901963553U, 18329021337179044916U,
};
KASSERT(NELEMS(sorted) == NELEMS(unsorted));

struct kl_elem
{
    kernel::klink   link;
    uint64_t        val;
    kl_elem(uint64_t val):val(val) {}
};

struct kl_le
{
    inline bool operator()(kernel::klink* l, kernel::klink* r) const
    {
        kl_elem* lke = container_of(l,kl_elem,link);
        kl_elem* rke = container_of(r,kl_elem,link);
        return lke->val <= rke->val;
    }
};

class tmock_test
{
    TMOCK_TEST(test_some_integers)
    {
        kernel::sort::quicksort(unsorted);
        for (size_t i=0; i < NELEMS(unsorted); ++i)
            kassert(unsorted[i] == sorted[i]);
    }

    TMOCK_TEST(test_some_integers_flex_array)
    {
        uint64_t _unsorted[NELEMS(unsorted)];
        for (size_t i=0; i<NELEMS(unsorted); ++i)
            _unsorted[i] = unsorted[NELEMS(unsorted) - 1 - i];
        kassert(NELEMS(_unsorted) == NELEMS(unsorted));

        kernel::sort::quicksort(_unsorted);
        for (size_t i=0; i < NELEMS(_unsorted); ++i)
            kassert(_unsorted[i] == sorted[i]);
    }

    TMOCK_TEST_EXPECT_FAILURE(test_parent_iter_begin_fails)
    {
        auto begin = kernel::begin(unsorted);
        heap_sort::parent_iter(begin,begin);
    }

    TMOCK_TEST(test_parent_iter)
    {
        auto begin = kernel::begin(unsorted);
        for (size_t i = 1; i < NELEMS(unsorted); ++i)
        {
            auto pos                 = begin + i;
            auto parent              = heap_sort::parent_iter(begin,pos);
            ptrdiff_t expected_index = (i-1)/2;
            kassert(parent - begin == expected_index);
        }
    }

    TMOCK_TEST(test_has_left_child)
    {
        auto begin = kernel::begin(unsorted);
        auto end   = kernel::end(unsorted);
        for (size_t i = 0; i < NELEMS(unsorted); ++i)
        {
            auto pos = begin + i;
            kassert(heap_sort::has_left_child(begin,end,pos)
                    == ((2*i + 1) < NELEMS(unsorted)));
        }
    }

    TMOCK_TEST(test_left_child_iter)
    {
        auto begin = kernel::begin(unsorted);
        for (size_t i = 0; i < NELEMS(unsorted); ++i)
        {
            auto pos                 = begin + i;
            auto left_child          = heap_sort::left_child_iter(begin,pos);
            ptrdiff_t expected_index = 2*i + 1;
            kassert(left_child - begin == expected_index);
        }
    }

    TMOCK_TEST(test_has_right_child)
    {
        auto begin = kernel::begin(unsorted);
        auto end   = kernel::end(unsorted);
        for (size_t i = 0; i < NELEMS(unsorted); ++i)
        {
            auto pos = begin + i;
            kassert(heap_sort::has_right_child(begin,end,pos)
                    == ((2*i + 2) < NELEMS(unsorted)));
        }
    }

    TMOCK_TEST(test_right_child_iter)
    {
        auto begin = kernel::begin(unsorted);
        for (size_t i = 0; i < NELEMS(unsorted); ++i)
        {
            auto pos                 = begin + i;
            auto right_child         = heap_sort::right_child_iter(begin,pos);
            ptrdiff_t expected_index = 2*i + 2;
            kassert(right_child - begin == expected_index);
        }
    }

    TMOCK_TEST(test_heapify)
    {
        auto begin = kernel::begin(unsorted);
        auto end   = kernel::end(unsorted);
        kassert(!heap_sort::is_heap(begin,end));
        heap_sort::heapify(begin,end);
        kassert(heap_sort::is_heap(begin,end));
    }

    TMOCK_TEST(test_heap_insert)
    {
        std::vector<uint64_t> v;
        for (auto val : unsorted)
        {
            v.push_back(val);
            heap_sort::insert(v.begin(),v.end(),kernel::greater<uint64_t>());
            kassert(heap_sort::is_heap(v.begin(),v.end()));
        }
    }

    TMOCK_TEST(test_heap_remove)
    {
        std::vector<uint64_t> v(kernel::begin(unsorted),kernel::end(unsorted));
        heap_sort::heapify(v.begin(),v.end());
        while (!v.empty())
        {
            auto max_val = v[0];
            heap_sort::remove(v.begin(),v.end(),v.begin(),
                              kernel::greater<uint64_t>());
            kassert(v[v.size()-1] == max_val);
            v.pop_back();
            kassert(heap_sort::is_heap(v.begin(),v.end()));
        }
    }

    TMOCK_TEST(test_heapsort)
    {
        kernel::sort::heapsort(unsorted);
        for (size_t i=0; i < NELEMS(unsorted); ++i)
            kassert(unsorted[i] == sorted[i]);
    }

    TMOCK_TEST(test_heapsort_empty)
    {
        uint64_t empty[0] = {};
        heap_sort::heapsort(empty,empty);
    }

    TMOCK_TEST(test_heapsort_one)
    {
        uint64_t one[] = {1234567};
        kernel::sort::heapsort(one);
        kassert(one[0] == 1234567);
    }

    TMOCK_TEST(test_heapsort_two)
    {
        uint64_t two[] = {1234567,3};
        kernel::sort::heapsort(two);
        kassert(two[0] == 3);
        kassert(two[1] == 1234567);
    }

    TMOCK_TEST(test_kl_mergesort)
    {
        kernel::klist<kl_elem> l;
        for (auto v : unsorted)
            l.push_back(&(new kl_elem(v))->link);
        kernel::sort::mergesort(l,kl_le());

        size_t i = 0;
        for (auto& kle : klist_elems(l,link))
        {
            tmock::assert_equiv(kle.val,sorted[i]);
            ++i;
        }
        tmock::assert_equiv(i,NELEMS(sorted));

        while (!l.empty())
        {
            auto* e = klist_front(l,link);
            l.pop_front();
            delete e;
        }
    }
};

TMOCK_MAIN();
