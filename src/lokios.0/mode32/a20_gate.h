#ifndef __MODE32_A20_GATE_H
#define __MODE32_A20_GATE_H

extern "C"
{
    void _a20_enable_int15h();
    int a20_enable();
}

#endif /* __MODE32_A20_GATE_H */
