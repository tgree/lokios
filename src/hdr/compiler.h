#ifndef __LOKIOS_COMPILER_H
#define __LOKIOS_COMPILER_H

#define __UNUSED__      __attribute__((unused))
#define __PACKED__      __attribute__((packed))
#define __ALIGNED__(n)  __attribute__((aligned(n)))
#define __PRINTF__(a,b) __attribute__((format(printf,a,b)))

#endif /* __LOKIOS_COMPILER_H */
