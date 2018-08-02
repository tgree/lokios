/*
 * Stack unwinder to print a backtrace.  See:
 *
 * http://refspecs.linux-foundation.org/LSB_4.1.0/LSB-Core-generic/LSB-Core-generic/libgcc-sman.html
 */
#include "kbacktrace.h"
#include "console.h"
#include <unwind.h>

static _Unwind_Reason_Code
unwind_one(_Unwind_Context* context, void* data)
{
    int ip_before_insn;
    uintptr_t pc = _Unwind_GetIPInfo(context,&ip_before_insn);
    if (!ip_before_insn)
        --pc;

    kernel::console::printf("  0x%016lX\n",pc);
    return _URC_NO_REASON;
}

void
kernel::backtrace(uintptr_t* pcs, size_t npcs)
{
    _Unwind_Backtrace(unwind_one,NULL);
}

void
kernel::init_backtrace()
{
}
