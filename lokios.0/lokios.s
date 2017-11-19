.code16
.text

.equiv DUMP_REGS, 0
.equiv PRINT_TEST, 0

# Entry point for an MBR boot.  On entry:
#   CS:IP - set to 0:7C00h.
#   DS    - 0
#   DL    - contains the drive number for use with INT 13h.
.globl _lokios_start_mbr
_lokios_start_mbr:
    lea     _mbr_text, %si
    jmp     .L_lokios_start

# Entry point for a PXE boot.  On entry:
#   CS:IP     - set to 0:7C00h.
#   ES:BX     - contains the address of the PXENV+ structure.
#   DS        - 0
#   SS:[SP+4] - contains the segment:offset address of the !PXE structure.
#   SS:SP     - we've already saved the entry register state on the stack, but
#               there should be a bit less than 1.5K of free space left
.globl _lokios_start_pxe
_lokios_start_pxe:
    lea     _pxe_text, %si

# Entry point for LokiOS.
#   %si - contains a pointer to an initial banner to print.
.L_lokios_start:
    # Print the initial banner.
    call    _puts

    # Print our banner.
    lea     _loki_os_banner, %si
    call    _puts

    # Print the build time.
    lea     _BUILD_TIME, %si
    call    _puts
    call    _putCRLF

.if PRINT_TEST
    mov     $0x789ABCDE, %edx
    call    _put32
    call    _putCRLF
    mov     $0x11CC, %dx
    call    _put16
    call    _putCRLF
.endif

.if DUMP_REGS
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
.endif

    ret


.if DUMP_REGS
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
.endif


# --------------------- Data Segment ---------------------
.data
.globl _loki_os_banner
_loki_os_banner:
    .ascii  "Loki\r\n"
    .asciz  "Copyright (c) 2017 by Terry Greeniaus.\r\n"
_mbr_text:
    .asciz  "MBR "
_pxe_text:
    .asciz  "PXE "

.if DUMP_REGS
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
.endif
