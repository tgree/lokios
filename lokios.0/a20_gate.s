.code16
.text


# Try to enable the A20 gate.  This gate controls the 20th address line.  If it
# is disabled, then the the A20 line is forced to 0 for all memory accesses.
# If it is enabled, then the A20 line will have the actual 20th bit of the
# address bus.  In order to access more than 1M of memory reliably, we need to
# disable this line.
.globl _A20_enable
_A20_enable:
    # Sanity check - if it already works don't do anything further.
    call    .L_A20_check
    jc      .L_A20_bios_enable
    ret

.L_A20_bios_enable:
    # Try enabling it through BIOS.  The carry bit propagates up.
    movw    $0x2401, %ax
    int     $0x15
    call    .L_A20_check
    ret

.L_A20_bios_failed:
    ret


# We are going to set .L_A20_sig, change it and then look at it again one
# MB higher.  If the values 
.L_A20_check:
    mov     $0x00100000, %eax
    movw    $0x1357, .L_A20_sig
    cmpw    $0x1357, %fs:.L_A20_sig(%eax)
    jne     .L_A20_check_enabled
    movw    $0x2468, .L_A20_sig
    cmpw    $0x2468, %fs:.L_A20_sig(%eax)
    jne     .L_A20_check_enabled
.L_A20_check_disabled:
    stc
    jmp     .L_A20_check_out
.L_A20_check_enabled:
    clc
.L_A20_check_out:
    mov     $0, %ax
    mov     %ax, %es
    ret


.data
.L_A20_sig:
    .word   0
