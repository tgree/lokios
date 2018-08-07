/*
 * Stack unwinder to print a backtrace.  See:
 *
 * http://refspecs.linux-foundation.org/LSB_4.1.0/LSB-Core-generic/LSB-Core-generic/libgcc-sman.html
 */
#include "kbacktrace.h"
#include "symtab.h"
#include "console.h"
#include <unwind.h>
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
            printf("  0x%016lX  %s+%zu\n",pc,si.name,si.offset);
        }
        catch (kernel::symbol_not_found_exception&)
        {
            printf("  0x%016lX\n",pc);
        }
    }
}
