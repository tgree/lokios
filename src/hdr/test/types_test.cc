#include "../types.h"
#include "../kassert.h"
#include "../compiler.h"
#include <stdint.h>
#include <tmock/tmock.h>

struct big_struct
{
    uint32_t    f4;
    uint8_t     f1;
    uint64_t    f8;
    uint8_t     f1234[1234];
    uint16_t    f2;
    uint128_t   f16;
};
KASSERT(sizeof_field(big_struct,f4)    == 4);
KASSERT(sizeof_field(big_struct,f1)    == 1);
KASSERT(sizeof_field(big_struct,f8)    == 8);
KASSERT(sizeof_field(big_struct,f1234) == 1234);
KASSERT(sizeof_field(big_struct,f2)    == 2);
KASSERT(sizeof_field(big_struct,f16)   == 16);
KASSERT(nelmof_field(big_struct,f1234) == 1234);

typeof_field(big_struct,f4)    g4;
typeof_field(big_struct,f1)    g1;
typeof_field(big_struct,f8)    g8;
typeof_field(big_struct,f1234) g1234;
typeof_field(big_struct,f2)    g2;
typeof_field(big_struct,f16)   g16;
KASSERT(sizeof(g4)          == 4);
KASSERT(sizeof(g1)          == 1);
KASSERT(sizeof(g8)          == 8);
KASSERT(sizeof(g1234)       == 1234);
KASSERT(sizeof(g2)          == 2);
KASSERT(sizeof(g16)         == 16);
KASSERT(nelmof_array(g1234) == 1234);

TMOCK_MAIN();
