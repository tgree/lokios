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

    # Print the banners.
    lea     _mbr_text, %si
    call    _dispatch_print_banners

    # Restore DL.
    popw    %dx

    # Load sector 2 from disk to 0:0x7E00.
    mov     $0, %ch         # cylinder 0
    mov     $2, %cl         # sector 2
    mov     $0, %dh         # head 0
    mov     $0x7E00, %bx    # ES:BX = 0:0x7E00
    call    _mbr_read_sector
    jc      _mbr_read_failed

    # First sector read succeeded.  Read all the remaining sectors.  We assume
    # fewer than 256 sectors.
.L_read_remaining_loop:
    movb    _last_sector, %al
    cmpb    %cl, %al
    je      .L_load_succeeded
    call    _mbr_next_sector
    call    _mbr_read_sector
    jc      _mbr_read_failed
    jmp     .L_read_remaining_loop

.L_load_succeeded:
    # Read succeeded.  Jump to second-stage loader.  If it returns, then we
    # will just bail back to BIOS.
    jmp     _mbr_entry


# Read a sector from disk.  On entry:
#   CH    - cylinder[7:0]
#   CL    - cylinder[9:8] | sector (1-based)
#   DH    - head
#   DL    - driver number
#   ES:BX - destination address
_mbr_read_sector:
    mov     $2, %ah         # opcode
    mov     $1, %al         # total sector count
    int     $0x13
    jc      .L_handy_ret
    cmp     $0, %ah
    je      .L_handy_ret
    stc
.L_handy_ret:
    ret


# Increment values to point to the next sector.
# TODO: In the future this should take geometry into account.
_mbr_next_sector:
    inc     %cl
    add     $512, %bx
    ret


# Print an error string and bail back to BIOS.
_mbr_read_failed:
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
    # Print the banners and jump straight into the second stage which has
    # already been loaded as part of the PXE image fetch.  If this returns it
    # will just bail straight back to BIOS.
    lea     _pxe_text, %si
    call    .L_dispatch_print_banners
    jmp     _pxe_entry


# Print hello world banners.
#   %si - contains a pointer to an initial banner to print.
_dispatch_print_banners:
    # Print the initial banner.
    call    _puts

    # Print the loki copyright banner.
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
