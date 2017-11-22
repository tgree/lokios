.code16
.text


# Entry point from the bootsector.
.globl _mbr_entry
_mbr_entry:
    # Note that we have successfully started the MBR bootloader.
    lea     .L_mbr_bootloader_started_text, %si
    call    _puts

    # Entry unreal mode.
    call    _enter_unreal_mode

    # TODO: Use BIOS to lokios.1 (the kernel) to address 2M.
    # For now, we simply write a HLT instruction there.
    mov     $_kernel_base, %eax
    movb    $0xF4, %fs:(%eax)

    # Jump to the common entry point.
    jmp     _common_entry


.data
.L_mbr_bootloader_started_text:
    .asciz  "MBR bootloader started\r\n"
