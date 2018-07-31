#ifndef __LOKIOS_CONTAINER_OF_H
#define __LOKIOS_CONTAINER_OF_H

#include <stddef.h>

#define container_of(p,type,field) \
    ((type*)((char*)(1 ? (p) : &((type *)0)->field) - offsetof(type,field)))

#endif /* __LOKIOS_CONTAINER_OF_H */
