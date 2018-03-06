#ifndef __CXXABI_H
#define __CXXABI_H

namespace abi
{
    char*
    __cxa_demangle(const char* __mangled_name, char* __output_buffer,
                   size_t* __length, int* __status);
}

#endif /* __CXXABI_H */
