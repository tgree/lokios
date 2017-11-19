.code16
.text

# Entry point for an MBR boot.  On entry:
#   CS:IP - set to 0:7C00h.
#   DS    - 0
#   DL    - contains the drive number for use with INT 13h.
.globl _dispatch_mbr
_dispatch_mbr:
    # Print the banners.
    lea     _mbr_text, %si
    call    .L_dispatch_print_banners

    # Bail back to BIOS.
    ret


# Entry point for a PXE boot.  On entry:
#   CS:IP     - set to 0:7C00h.
#   ES:BX     - contains the address of the PXENV+ structure.
#   DS        - 0
#   SS:[SP+4] - contains the segment:offset address of the !PXE structure.
#   SS:SP     - we've already saved the entry register state on the stack, but
#               there should be a bit less than 1.5K of free space left
.globl _dispatch_pxe
_dispatch_pxe:
    # Print the banners.
    lea     _pxe_text, %si
    call    .L_dispatch_print_banners

    # Bail back to BIOS.
    ret


# Entry point for LokiOS.
#   %si - contains a pointer to an initial banner to print.
.L_dispatch_print_banners:
    # Print the initial banner.
    call    _puts

    # Print our banner.
    lea     _loki_os_banner, %si
    call    _puts

    # Print the build time.
    lea     _BUILD_TIME, %si
    call    _puts

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
