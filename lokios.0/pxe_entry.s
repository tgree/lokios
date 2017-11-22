.code16
.text


# Entry point from the bootsector.
.globl _pxe_entry
_pxe_entry:
    # Note that we have successfully started the PXE bootloader.
    lea     .L_pxe_bootloader_started_text, %si
    call    _puts

    # Entry unreal mode.
    call    _enter_unreal_mode

    # TODO: PXE fetch lokios.1 (the kernel) to address 2M.
    # For now, we simply write a HLT instruction there.
    mov     $_kernel_entry, %eax
    movb    $0xF4, %fs:(%eax)

    # Jump to the common entry point.
    jmp     _common_entry


.data
.L_pxe_bootloader_started_text:
    .asciz  "PXE bootloader started\r\n"
