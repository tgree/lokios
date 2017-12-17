#ifndef __KERNEL_CXX_EXCEPTION_H
#define __KERNEL_CXX_EXCEPTION_H

namespace kernel
{
    struct exception
    {
        virtual const char* c_str() const = 0;
        operator const char*() const {return c_str();}
    };

    void throw_test_exception();
}

#endif /* __KERNEL_CXX_EXCEPTION_H */
