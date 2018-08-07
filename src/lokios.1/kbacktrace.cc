/*
 * Stack unwinder to print a backtrace.  See:
 *
 * http://refspecs.linux-foundation.org/LSB_4.1.0/LSB-Core-generic/LSB-Core-generic/libgcc-sman.html
 */
#include "kbacktrace.h"
#include "symtab.h"
#include "console.h"
#include <unwind.h>
#include <cxxabi.h>
#include <stdlib.h>

using kernel::console::printf;

struct unwind_state
{
    uintptr_t*  pos;
    uintptr_t   pcs[16];
};

extern uint8_t _virt_addr[];

static _Unwind_Reason_Code
unwind_one(_Unwind_Context* context, void* data)
{
    int ip_before_insn;
    uintptr_t pc = _Unwind_GetIPInfo(context,&ip_before_insn);
    if (pc < (uintptr_t)_virt_addr)
        return _URC_END_OF_STACK;
    if (!ip_before_insn)
        --pc;

    auto* us   = (unwind_state*)data;
    *us->pos++ = pc;
    if (us->pos == us->pcs + NELEMS(us->pcs))
        return _URC_END_OF_STACK;

    return _URC_NO_REASON;
}

void
kernel::backtrace(const char* header)
{
    if (header)
        printf("%s:\n",header);

    unwind_state us;
    us.pos = us.pcs;
    _Unwind_Backtrace(unwind_one,&us);

    us.pos--;
    while (us.pos > us.pcs)
    {
        uintptr_t pc = *us.pos--;
        try
        {
            kernel::sym_info si = kernel::get_sym_info((void*)pc);
            if (si.addr == (uintptr_t)kernel::panic ||
                si.addr == (uintptr_t)::abort)
            {
                break;
            }

            // This is a hack to demangle without allocating memory.  If we
            // don't pass in a buffer, __cxa_demangle will malloc one.  If we
            // do pass in a buffer and it's too small, __cxa_demangle will
            // either a realloc on it or a free/malloc.  In either case, we are
            // screwed because on this path we don't want to rely on memory
            // allocation anymore (stuff could be badly corrupt).
            //
            // Our crappy malloc can only malloc up to 4K before itself
            // asserting (which could be a double-assert on this code path if
            // we are dumping a backtrace from abort handling!), so we just do
            // a 4K buffer on the stack and let free or realloc blow up if we
            // really do have a symbol longer than 4K.
            int status;
            char buf[4096];
            size_t n = sizeof(buf);
            abi::__cxa_demangle(si.name,buf,&n,&status);
            if (!status)
                printf("  0x%016lX  %s+%zu/%zu\n",pc,buf,si.offset,si.size);
            else
                printf("  0x%016lX  %s+%zu/%zu\n",pc,si.name,si.offset,si.size);
        }
        catch (kernel::symbol_not_found_exception&)
        {
            printf("  0x%016lX\n",pc);
        }
    }
}
