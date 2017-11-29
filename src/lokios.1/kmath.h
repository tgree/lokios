#ifndef __KERNEL_MATH_H
#define __KERNEL_MATH_H

template<typename T>
inline T min(T l, T r)
{
    return (l < r ? l : r);
}

template<typename T>
inline T max(T l, T r)
{
    return (l < r ? r : l);
}

#endif /* __KERNEL_MATH_H */
