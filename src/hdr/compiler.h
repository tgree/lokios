#ifndef __LOKIOS_COMPILER_H
#define __LOKIOS_COMPILER_H

#define __UNUSED__      __attribute__((unused))
#define __PACKED__      __attribute__((packed))
#define __ALIGNED__(n)  __attribute__((aligned(n)))
#define __PRINTF__(a,b) __attribute__((format(printf,a,b)))
#define __NORETURN__    __attribute__((noreturn))

#define __CACHE_ALIGNED__   __ALIGNED__(64)

typedef __uint128_t uint128_t;

#define __COMPILER_BARRIER__()  asm volatile("":::"memory")

#endif /* __LOKIOS_COMPILER_H */
