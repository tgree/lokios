.code16
.text

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

    ret


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
