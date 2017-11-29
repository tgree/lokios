.code16
.text


.equiv _first_kernel_sector, _extra_sectors+1

# Entry point from the bootsector.
.globl _mbr_entry
_mbr_entry:
    # Note that we have successfully started the MBR bootloader and enable both
    # the A20 gate and unreal mode.
    lea     .L_mbr_bootloader_started_text, %si
    call    _common_init
    jc      .L_mbr_entry_return_to_bios

    # We are going to read the kernel into a bounce buffer and then copy it
    # into high memory.  If the kernel is larger than the size of the bounce
    # buffer, then we may need multiple operations to do the whole image.
    
    # Set up our "globals".  These registers are preserved across all the
    # function calls we make.
    mov     $_first_kernel_sector, %ebx
    mov     $_kernel_base, %edi
    mov     _mbr_drive_number, %dl

    # Fetch the first sector of the kernel into the bounce buffer.
    mov     $1, %cx
    lea     _pre_e820_bounce_buffer, %si
    call    _disk_read
    jc      .L_disk_read_failed

    # Get the total number of sectors.  This is stored as a 32-bit value at the
    # beginning of the first kernel sector.
    movl    _pre_e820_bounce_buffer, %esi
    push    %esi

    # Loop over all the sectors.  We do up to 4K at a time.
.L_read_loop:
    # See if we are done.
    pop     %esi
    test    %esi, %esi
    je      .L_read_loop_done

    # Figure out how many sectors to read and push the number of remaining
    # sectors for the next iteration.
    mov     %esi, %ecx
    cmp     $8, %esi
    jb      .L_below_8_to_go
    mov     $8, %ecx
.L_below_8_to_go:
    sub     %ecx, %esi
    push    %esi

    # Read them into the bounce buffer.
    lea     _pre_e820_bounce_buffer, %si
    call    _disk_read
    jc      .L_disk_read_failed_pop

    # Increment the sector number.
    add     %ecx, %ebx

    # Copy the bounce buffer into high memory.  This also advances EDI for us.
    lea     _pre_e820_bounce_buffer, %esi
    shl     $7, %ecx
    call    _unreal_memcpy

    # Loop.
    jmp     .L_read_loop

.L_read_loop_done:
    # Jump to the common entry point.
    jmp     _common_entry

.L_disk_read_failed_pop:
    pop     %esi
.L_disk_read_failed:
    lea     .L_disk_read_failed_text, %si
    call    _puts
.L_mbr_entry_return_to_bios:
    ret


.data
.L_mbr_bootloader_started_text:
    .asciz  "MBR bootloader started\r\n"
.L_disk_read_failed_text:
    .asciz  "Disk read failed\r\n"
