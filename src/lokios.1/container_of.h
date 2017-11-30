#ifndef __KERNEL_CONTAINER_OF_H
#define __KERNEL_CONTAINER_OF_H

#include <stddef.h>

#define container_of(p,type,field) \
    ((type*)((char*)(1 ? (p) : &((type *)0)->field) - offsetof(type,field)))

#endif /* __KERNEL_CONTAINER_OF_H */
