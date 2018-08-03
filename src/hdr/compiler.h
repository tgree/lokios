#ifndef __LOKIOS_COMPILER_H
#define __LOKIOS_COMPILER_H

#define __UNUSED__      __attribute__((unused))
#define __PACKED__      __attribute__((packed))
#define __ALIGNED__(n)  __attribute__((aligned(n)))
#define __PRINTF__(a,b) __attribute__((format(printf,a,b)))
#define __NORETURN__    __attribute__((noreturn))
#define __FASTCALL__    __attribute__((fastcall))
#define __REG_PARMS__   __attribute__((regparm(3)))

#define __CACHE_ALIGNED__   __ALIGNED__(64)

#if __SIZEOF_POINTER__ == 8
typedef __uint128_t uint128_t;
#endif

#define __COMPILER_BARRIER__()  asm volatile("":::"memory")

#define DO_PRAGMA(x) _Pragma(#x)
#define TODO(x) DO_PRAGMA(message("\033[31mTODO: " x "\033[0m"))

#endif /* __LOKIOS_COMPILER_H */
