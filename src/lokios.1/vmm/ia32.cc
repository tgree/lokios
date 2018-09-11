#include "ia32.h"

void
vmm::ia32_cpu::hard_reset()
{
    rflags = 0x00000002;
    rip    = 0x0000FFF0;
    cr[0]  = 0x60000010;
    cr[2]  = 0x00000000;
    cr[3]  = 0x00000000;
    cr[4]  = 0x00000000;
    
    srs.cs.selector = 0xF000;
    srs.cs.base     = 0xFFFF0000;
    srs.cs.limit    = 0x0000FFFF;

    srs.ss.selector = 0x0000;
    srs.ss.base     = 0x00000000;
    srs.ss.limit    = 0x0000FFFF;
    srs.ds.selector = 0x0000;
    srs.ds.base     = 0x00000000;
    srs.ds.limit    = 0x0000FFFF;
    srs.es.selector = 0x0000;
    srs.es.base     = 0x00000000;
    srs.es.limit    = 0x0000FFFF;
    srs.fs.selector = 0x0000;
    srs.fs.base     = 0x00000000;
    srs.fs.limit    = 0x0000FFFF;
    srs.gs.selector = 0x0000;
    srs.gs.base     = 0x00000000;
    srs.gs.limit    = 0x0000FFFF;

    gprs.edx   = 0x000F06FF; // TODO: values for F
    gprs.eax   = 0;
    gprs.ebx   = 0;
    gprs.ecx   = 0;
    gprs.esi   = 0;
    gprs.edi   = 0;
    gprs.ebp   = 0;
    gprs.esp   = 0;
    gprs.r[8]  = 0;
    gprs.r[9]  = 0;
    gprs.r[10] = 0;
    gprs.r[11] = 0;
    gprs.r[12] = 0;
    gprs.r[13] = 0;
    gprs.r[14] = 0;
    gprs.r[15] = 0;

    gdtr.base  = 0x00000000;
    gdtr.limit = 0xFFFF;
    idtr.base  = 0x00000000;
    idtr.limit = 0xFFFF;

    dr[0] = 0x00000000;
    dr[1] = 0x00000000;
    dr[2] = 0x00000000;
    dr[3] = 0x00000000;
    dr[6] = 0xFFFF0FF0;
    dr[7] = 0x00000400;

    xcr[0] = 1;
}
