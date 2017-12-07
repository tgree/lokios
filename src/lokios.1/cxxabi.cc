#include "kassert.h"
#include "mm/page.h"
#include <stddef.h>
#include <new>

void*
operator new(std::size_t count)
{
    kernel::kassert(count <= PAGE_SIZE);
    return kernel::page_alloc();
}

void
operator delete(void* p)
{
    kernel::kassert(((uintptr_t)p & PAGE_OFFSET_MASK) == 0);
    if (p)
        kernel::page_free(p);
}

void
operator delete(void* p, std::size_t sz)
{
    kernel::kassert(((uintptr_t)p & PAGE_OFFSET_MASK) == 0);
    kernel::kassert(sz <= PAGE_SIZE);
    if (p)
        kernel::page_free(p);
}

extern "C" void
__cxa_pure_virtual()
{
    // This will be invoked when a pure virtual function is called.  It can
    // print message and then should abort.
    kernel::panic("Pure virtual method invoked.");
}

extern "C" int
__cxa_atexit(void (*destructor)(void*), void* arg, void* dso)
{
    // This is called by global constructors after they successfully
    // instantiate their global object.  The object needs to be destroyed when
    // the runtime exits, so it uses this function to register for a destructor
    // call.  Since we never unload the kernel, we don't implement this and
    // global destructors will never be called.
    return -1;
}

extern "C" void
__stack_chk_fail()
{
    // This is called when a library has detected that a stack variable has
    // been stomped.  We get these when -fstack-protecter is enabled (this is
    // the default for g++).  Unfortunately -fstack-protector also uses
    // FS-based accesses to get the randomized stack stomp magic value, so we
    // have it disabled in the Makefile for now.
    kernel::panic("Stack stomp check failed.");
}

extern "C" int
__sprintf_chk(char* str, int flag, size_t strlen, const char* format)
{
    // The _FORTIFY_SOURCE macro triggers generation of a number of checked
    // alternatives for some standard library functions.  This one is required
    // by libsupc++.
    kernel::panic("__sprintf_chk");
}
