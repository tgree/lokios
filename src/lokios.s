.code16
.text


.globl _lokios_start_mbr
_lokios_start_mbr:
    lea     _mbr_text, %si
    call    _puts
    jmp     .L_lokios_start

.globl _lokios_start_pxe
_lokios_start_pxe:
    lea     _pxe_text, %si
    call    _puts
    jmp     .L_lokios_start

# Entry point for LokiOS.  Upon entry, system state is as follows:
#
#   1. We are running in real mode.
#   2. CS = ES = DS = 0.
#   3. SS is in some BIOS buffer.
#   4. The register state has been saved on the stack.
#   5. We were invoked via a call and can ret back to BIOS if we want to bail.
.L_lokios_start:
    # Print our banner.
    lea     _loki_os_banner, %si
    call    _puts

    # Print the contents of some interesting registers.
    lea     _cr0_text, %si
    mov     %cr0, %edx
    call    _dump_reg32

    mov     %sp, %bp
    lea     _gs_text, %si
    movw    4(%bp),%dx
    call    _dump_reg16
    lea     _gs_text, %si
    mov     %gs, %dx
    call    _dump_reg16

    lea     _fs_text, %si
    movw    6(%bp),%dx
    call    _dump_reg16
    lea     _fs_text, %si
    mov     %fs, %dx
    call    _dump_reg16

    lea     _es_text, %si
    movw    8(%bp),%dx
    call    _dump_reg16
    lea     _es_text, %si
    mov     %es,%dx
    call    _dump_reg16

    lea     _ds_text, %si
    movw    10(%bp),%dx
    call    _dump_reg16
    lea     _ds_text, %si
    mov     %ds,%dx
    call    _dump_reg16

    lea     _cs_text, %si
    mov     %cs, %dx
    call    _dump_reg16

    lea     _ss_text, %si
    mov     12(%bp), %dx
    call    _dump_reg16
    lea     _ss_text, %si
    mov     %ss, %dx
    call    _dump_reg16

    ret


# Print a label and a 32-bit value:
#   %si  - label
#   %edx - value
_dump_reg32:
    push    %edx
    call    _puts
    pop     %edx
    call    _put32
    jmp     _putCRLF

# Print a label and a 16-bit value:
#   %si - label
#   %dx - value
_dump_reg16:
    push    %dx
    call    _puts
    pop     %dx
    call    _put16
    jmp     _putCRLF


# --------------------- Data Segment ---------------------
.data
.globl _loki_os_banner
_loki_os_banner:
    .ascii  "Loki OS\r\n"
    .asciz  "Copyright (c) 2017 by Terry Greeniaus.  All rights reserved.\r\n"
_mbr_text:
    .asciz  "MBR"
_pxe_text:
    .asciz  "PXE"
_cr0_text:
    .asciz  "CR0: "
_gs_text:
    .asciz  "GS:  "
_fs_text:
    .asciz  "FS:  "
_es_text:
    .asciz  "ES:  "
_ds_text:
    .asciz  "DS:  "
_cs_text:
    .asciz  "CS:  "
_ss_text:
    .asciz  "SS:  "
