#include "../ia32.h"
#include <tmock/tmock.h>

class tmock_test
{
    TMOCK_TEST(test_register_layout)
    {
        vmm::ia32_gprs gprs;
        gprs.rax   = 0x0000000011112233;
        gprs.rbx   = 0x1111111122223344;
        gprs.rcx   = 0x2222222233334455;
        gprs.rdx   = 0x3333333344445566;
        gprs.rbp   = 0x4444444455556666;
        gprs.rsi   = 0x5555555566667777;
        gprs.rdi   = 0x6666666677778888;
        gprs.rsp   = 0x7777777788889999;
        gprs.r[8]  = 0x888888889999AAAA;
        gprs.r[9]  = 0x99999999AAAABBBB;
        gprs.r[10] = 0xAAAAAAAABBBBCCCC;
        gprs.r[11] = 0xBBBBBBBBCCCCDDDD;
        gprs.r[12] = 0xCCCCCCCCDDDDEEEE;
        gprs.r[13] = 0xDDDDDDDDEEEEFFFF;
        gprs.r[14] = 0xEEEEEEEEFFFF0000;
        gprs.r[15] = 0xFFFFFFFF00001111;

        tmock::assert_equiv(gprs.eax,0x11112233U);
        tmock::assert_equiv(gprs.ax,0x2233U);
        tmock::assert_equiv(gprs.ah,0x22U);
        tmock::assert_equiv(gprs.al,0x33U);

        tmock::assert_equiv(gprs.ebx,0x22223344U);
        tmock::assert_equiv(gprs.bx,0x3344U);
        tmock::assert_equiv(gprs.bh,0x33U);
        tmock::assert_equiv(gprs.bl,0x44U);

        tmock::assert_equiv(gprs.ecx,0x33334455U);
        tmock::assert_equiv(gprs.cx,0x4455U);
        tmock::assert_equiv(gprs.ch,0x44U);
        tmock::assert_equiv(gprs.cl,0x55U);

        tmock::assert_equiv(gprs.edx,0x44445566U);
        tmock::assert_equiv(gprs.dx,0x5566U);
        tmock::assert_equiv(gprs.dh,0x55U);
        tmock::assert_equiv(gprs.dl,0x66U);

        tmock::assert_equiv(gprs.ebp,0x55556666U);
        tmock::assert_equiv(gprs.bp,0x6666U);

        tmock::assert_equiv(gprs.esi,0x66667777U);
        tmock::assert_equiv(gprs.si,0x7777U);

        tmock::assert_equiv(gprs.edi,0x77778888U);
        tmock::assert_equiv(gprs.di,0x8888U);

        tmock::assert_equiv(gprs.esp,0x88889999U);
        tmock::assert_equiv(gprs.sp,0x9999U);
    }
};

TMOCK_MAIN();
