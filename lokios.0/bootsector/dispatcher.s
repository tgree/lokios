.code16
.text

# Dispatcher to load the rest of the bootloader from disk.
.globl _dispatch_mbr
_dispatch_mbr:
    # Restore DL.
    mov     _mbr_drive_number, %dl

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


# --------------------- Data Segment ---------------------
.data
_mbr_failed_text:
    .asciz  "MBR boot failed\r\n"
