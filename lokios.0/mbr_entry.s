.code16
.text


.equiv _first_kernel_sector, _extra_sectors+1

# Entry point from the bootsector.
.globl _mbr_entry
_mbr_entry:
    # Note that we have successfully started the MBR bootloader.
    lea     .L_mbr_bootloader_started_text, %si
    call    _puts

    # Entry unreal mode.
    call    _enter_unreal_mode

    # Fetch the first sector of the kernel into the bounce buffer.
    mov     _mbr_drive_number, %dl
    mov     $_first_kernel_sector, %eax
    mov     $0, %ebx
    mov     $1, %cx
    lea     _pre_e820_bounce_buffer, %si
    call    _disk_read

    # Copy the bounce buffer sector into high memory.
    lea     _pre_e820_bounce_buffer, %eax
    mov     $_kernel_base, %edx
    mov     $512/4, %ecx
    call    _unreal_memcpy

    # Jump to the common entry point.
    jmp     _common_entry


.data
.L_mbr_bootloader_started_text:
    .asciz  "MBR bootloader started\r\n"
