.code16
.text

# Entry point for an MBR boot.  On entry:
#   CS:IP - set to 0:7C00h.
#   DS    - 0
#   DL    - contains the drive number for use with INT 13h.
.globl _dispatch_mbr
_dispatch_mbr:
    # Save off DL.
    pushw   %dx

    # Zero ES because we need it to point to the same place as DS.
    xor     %dx, %dx
    mov     %dx, %es

    # Print the banners.
    lea     _mbr_text, %si
    call    .L_dispatch_print_banners

    # Now we need to load the first sector lokios.1 at 0x7e00.
    mov     $2, %ah         # opcode
    mov     $1, %al         # total sector count
    mov     $0, %ch         # cylinder[7:0]
    mov     $2, %cl         # cylinder[9:8] | sector (1-based)
    popw    %dx             # drive number in DL
    mov     $0, %dh         # head in DH
    mov     $0x7E00, %bx    # ES:BX = 0:0x7E00
    int     $0x13
    jc      .L_mbr_read_failed
    cmp     $0, %ah
    jne     .L_mbr_read_failed
.L_mbr_read_succeeded:
    lea     _puts, %ax
    jmp     0x7E10

.L_mbr_read_failed:
    # Print an error string and bail back to BIOS.
    lea     _mbr_failed_text, %si
    call    _puts
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
_mbr_failed_text:
    .asciz  "MBR boot failed\r\n"
