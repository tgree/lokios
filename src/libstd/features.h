#include "lokios.1/kassert.h"

typedef char int8_t;
typedef unsigned char uint8_t;
KASSERT(sizeof(int8_t) == 1);
KASSERT(sizeof(uint8_t) == 1);

typedef short int16_t;
typedef unsigned short uint16_t;
KASSERT(sizeof(int16_t) == 2);
KASSERT(sizeof(uint16_t) == 2);

typedef int int32_t;
typedef unsigned int uint32_t;
KASSERT(sizeof(int32_t) == 4);
KASSERT(sizeof(uint32_t) == 4);

typedef long int64_t;
typedef unsigned long uint64_t;
KASSERT(sizeof(int64_t) == 8);
KASSERT(sizeof(uint64_t) == 8);

typedef uint64_t uintptr_t;

typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
