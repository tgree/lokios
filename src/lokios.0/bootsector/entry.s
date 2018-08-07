.code16
.text


# Starting point.  These are the first instructions executed after BIOS and we
# are in the wild wild west.  Since we want to be able to handle either an MBR
# or a PXE boot, we need to work with the lowest common denominator until we
# get more information about the state of the system.  With PXE we at least get
# a stack, but with MBR we don't even get that.  The state is something like
# this:
#
#   MBR:
#       CR0       - 0x60000010 (via bochs)
#       CS:IP     - 0:0x7C00 or possibly 0x7C0:0
#       DL        - contains the drive number in an MBR boot
#
#   PXE:
#       CR0       - 0x00000012 (via ipxe)
#       CS:IP     - supposed to be 0:0x7C00
#       ES:BX     - contains the PXENV+ struct
#       SS:[SP+4] - contains the segment:offset address of the !PXE struct
#       SS:SP     - contains 1.5K of stack
#
# Apparently there is no guarantee via MBR that there is any sort of usable
# stack.  So, we are going to preserve DL, ES:BX, SS:SP and then switch to our
# own stack.
# 
# osdev.org says that 0x07E00-0x7FFFF is guaranteed free for use.
# The PXE spec says that the max safe size of an image laoded at 0x7C00 is 32K,
# implying that only 0x07E00-0x0x0FC00 is guaranteed free for use.  Since PXE
# is the lowest common denominator here, we are going to relocate our stack to
# 0xFC00.
.globl _start
_start:
    # Clear CS
    ljmp    $0, $.L_start_clear_cs
.L_start_clear_cs:

    # Save SS:SP.
    movw    %sp, %cs:_saved_ss_sp
    movw    %ss, %sp
    movw    %sp, %cs:_saved_ss_sp+2

    # Save ES:BX
    mov     %bx, %cs:_saved_es_bx
    mov     %es, %bx
    mov     %es, %cs:_saved_es_bx+2

    # Save the drive number.
    mov     %dl, %cs:_mbr_drive_number

    # Load 0:FC00 into SS:SP.
    lss     %cs:.L_loader_ss_sp, %sp

    # Save all other args.
    pushfw
    pushaw
    pushw   %ds
    pushw   %fs
    pushw   %gs

    # Clear DS.
    xor     %ax, %ax
    mov     %ax, %ds

    # Clear direction so string operations move forward in memory.
    cld

    # Load the gdt.
    lgdt    _gdt_desc

    # Print the copyright banner first.
    lea     _BUILD_BANNER, %si
    call    _puts

    # Check if we can see a 'PXENV+' signature at ES:BX and dispatch to the
    # correct entry point.  We also set ES = 0 on the way out so that it maps
    # to the same segment as DS.
    xor     %ax, %ax
    lea     .L_pxenv_str, %si
    mov     _saved_es_bx, %di
    mov     $6, %cx
    repe cmpsb
    mov     %ax, %es
    jne     .L_mbr_start

    # PXE detected.  Just jump into the PXE entry point since the image was
    # already loaded for us as part of the PXE download.
.L_pxe_start:
    mov     $0x00000001, %eax
.L_call_m32_entry:
    call    _m32_entry
    jmp     .L_back_to_bios

    # MBR detected.  Read the remaining lokios.0 sectors from the disk since
    # BIOS has only loaded the first sector of the image.
.L_mbr_start:
    mov     _mbr_drive_number, %dl
    mov     $1, %ebx
    mov     $_extra_sectors, %cx
    mov     $0x7E00, %si
    call    _disk_read
    jc      .L_mbr_read_failed
    xor     %eax, %eax
    jmp     .L_call_m32_entry

.L_mbr_read_failed:
    lea     .L_mbr_read_failed_text, %si
    call    _puts
.L_back_to_bios:
    # Return to BIOS.
    pop     %gs
    pop     %fs
    pop     %ds
    popaw
    xor     %ax, %ax    # Return 0 to PXE BIOS.
    popfw
    les     %cs:_saved_es_bx, %bx
    lss     %cs:_saved_ss_sp, %sp
    lret


.data
.L_pxenv_str:
    .ascii  "PXENV+"
.L_mbr_read_failed_text:
    .asciz  "MBR read failed\r\n"


.L_loader_ss_sp:
    .word   _stack_top  # SP
    .word   0           # SS

.globl _saved_es_bx
_saved_es_bx:
    .word   0xAAAA  # BX
    .word   0xAAAA  # ES

.globl _saved_ss_sp
_saved_ss_sp:
    .word   0xBBBB  # SP
    .word   0xBBBB  # SS

.globl _mbr_drive_number
_mbr_drive_number:
    .byte   0xCC

# The GDT is an array of 64-bit values and it is pointed to by a 48-bit
# structure.  The first entry of the GDT is unused, so we stuff the GDT pointer
# in there and when calculating the GDT address subtract 2 bytes to make it
# a "full" 64-bit entry.  So the start of the GDT actually points a couple
# bytes before the beginning of this struct!
.globl _gdt_desc
_gdt_desc:
    .word   _gdt_desc_end - _gdt_desc + 2 - 1
    .long   _gdt_desc - 2
    .quad   0x00CF92000000FFFF # 32-bit data segment
    .quad   0x00CF9A000000FFFF # 32-bit code segment
    .quad   0x000092000000FFFF # 16-bit data segment
    .quad   0x00009A000000FFFF # 16-bit code segment
    .quad   0x0000920000000000 # 64-bit data segment
    .quad   0x00209A0000000000 # 64-bit code segment
_gdt_desc_end:
