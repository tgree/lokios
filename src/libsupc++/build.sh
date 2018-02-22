#!/bin/bash
for i in /usr/src/gcc-7/gcc-7.2.0/libstdc++-v3/libsupc++/*.cc
    do ln -s $i
done
for i in /usr/src/gcc-7/gcc-7.2.0/libstdc++-v3/libsupc++/*.h
    do ln -s $i
done
ln -s /usr/src/gcc-7/gcc-7.2.0/libiberty/cp-demangle.c

g++ -std=gnu++17 -mcmodel=kernel -O3 -Wno-unused-result -I/usr/src/gcc-7/gcc-7.2.0/libgcc -I/usr/src/gcc-7/gcc-7.2.0/include -c *.cc
gcc -mcmodel=kernel -I/usr/src/gcc-7/gcc-7.2.0/libgcc -I/usr/src/gcc-7/gcc-7.2.0/include -O3 -DHAVE_STRING_H -DHAVE_DECL_BASENAME -DHAVE_STDLIB_H -DIN_GLIBCPP_V3 -c *.c
ar rc libsupc++.a *.o
