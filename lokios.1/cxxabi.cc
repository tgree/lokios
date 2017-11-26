#include "assert.h"
#include <new>

void
operator delete(void* ptr, std::size_t sz)
{
    // We don't support dynamic memory allocations at this time.  Apparently
    // we get two versions of destructors created - one which is used when we
    // need to delete the object and one which is used when it is an in-place
    // destruction.  Despite never deleting anything the linker still needs to
    // be satisfied.
    aborts("operator delete() invoked.");
}

extern "C" void
__cxa_pure_virtual()
{
    // This will be invoked when a pure virtual function is called.  It can
    // print message and then should abort.
    aborts("Pure virtual method invoked.");
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
    aborts("Stack stomp check failed.");
}

extern "C" int
__sprintf_chk(char* str, int flag, size_t strlen, const char* format)
{
    // The _FORTIFY_SOURCE macro triggers generation of a number of checked
    // alternatives for some standard library functions.  This one is required
    // by libsupc++.
    aborts("__sprintf_chk");
}
