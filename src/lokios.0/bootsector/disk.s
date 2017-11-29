.code16
.text

# Read from disk into low memory.  If you want to move the output into a higher
# address you'll have to do it yourself through a bounce buffer.  Apparently
# some BIOSes can't read more than 127 sectors so you may want to throttle
# here anyways.
# On entry:
#   DL  - drive number
#   EBX - first sector low-order 32-bits
#   CX  - sector count
#   SI  - destination address
# On exit:
#   CF - set on failure, cleared on success
# Stomped:
#   EA[XHL], SI
.globl _disk_read
_disk_read:
    mov     %ebx, .L_DAP_first_sector_low
    mov     %cx, .L_DAP_num_sectors
    mov     %si, .L_DAP_destination_ptr
    mov     $.L_DAP, %si
    mov     $0x42, %ah
    int     $0x13
    ret


.data
.L_DAP:
    .byte   0x10
    .byte   0
.L_DAP_num_sectors:
    .word   0
.L_DAP_destination_ptr:
    .word   0
    .word   0
.L_DAP_first_sector_low:
    .long   0x11111111
.L_DAP_first_sector_high:
    .long   0   # Always 0 for our purposes since we don't seek very far
