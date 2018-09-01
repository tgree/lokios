#include <ctype.h>
#include <stdio.h>
#include <stdint.h>

static void
update(int i, __uint128_t* v, int (*checker)(int))
{
    if (checker(i))
        *v |= ((__uint128_t)1 << i);
}

#define PRINT(v) \
    printf("uint128_t " #v " = 0x%016lX%016lX;\n", \
           (uint64_t)(v >> 64),(uint64_t)v)

int
main(int argc, const char* argv[])
{
    __uint128_t alnum = 0;
    __uint128_t alpha = 0;
    __uint128_t cntrl = 0;
    __uint128_t digit = 0;
    __uint128_t graph = 0;
    __uint128_t lower = 0;
    __uint128_t print = 0;
    __uint128_t punct = 0;
    __uint128_t space = 0;
    __uint128_t upper = 0;
    __uint128_t xdigit = 0;
    __uint128_t ascii = 0;
    __uint128_t blank = 0;

    for (int i=0; i<128; ++i)
    {
        update(i,&alnum,isalnum);
        update(i,&alpha,isalpha);
        update(i,&cntrl,iscntrl);
        update(i,&digit,isdigit);
        update(i,&graph,isgraph);
        update(i,&lower,islower);
        update(i,&print,isprint);
        update(i,&punct,ispunct);
        update(i,&space,isspace);
        update(i,&upper,isupper);
        update(i,&xdigit,isxdigit);
        update(i,&ascii,isascii);
        update(i,&blank,isblank);
    }

    PRINT(alnum);
    PRINT(alpha);
    PRINT(cntrl);
    PRINT(digit);
    PRINT(graph);
    PRINT(lower);
    PRINT(print);
    PRINT(punct);
    PRINT(space);
    PRINT(upper);
    PRINT(xdigit);
    PRINT(ascii);
    PRINT(blank);
}
