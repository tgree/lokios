.code16
.text


# Entry point from the bootsector.
# On entry:
#   _saved_es_bx : should contain a far pointer to the PXENV+ structure
#   _saved_ss_sp : contains a far pointer to the entry stack
# The entry stack is useful because for later versions of the PXE spec it tells
# us how to find the !PXE structure.
.globl _pxe_entry
_pxe_entry:
    # Note that we have successfully started the PXE bootloader.
    lea     .L_pxe_bootloader_started_text, %si
    call    _puts

    # Entry unreal mode.
    call    _enter_unreal_mode

    # If we are here, we've already found that there is a PXENV+ signature at
    # ES:BX.  Load into GS:BX (we want to keep ES = DS) and let's checksum the
    # thing.  The PXENV+ structure is 44 bytes long.
    lgs     _saved_es_bx, %bx
    push    %bx
    xor     %ecx, %ecx
    movb    %gs:8(%bx), %cl
    call    _checksum
    test    %al, %al
    jne     _pxenv_checksum_mismatch

    # If the PXENV+ struct is less than 0x0201 then we should use it.
    pop     %bx
    cmpw    $0x0201, %gs:6(%bx)
    jb      _pxenv_below_2_1

    # There is a !PXE struct pointed to by the end of the PXENV+ struct.  Start
    # by looking for the "!PXE" signature.
    lgs     %gs:40(%bx), %bx
    cmpl    $0x45585021, %gs:(%bx)
    jne     _notpxe_sig_missing

    # Now we should checksum the !PXE struct.
    push    %bx
    xor     %ecx, %ecx
    movb    %gs:4(%bx), %cl
    call    _checksum
    test    %al, %al
    jne     _notpxe_checksum_mismatch

    # The !PXE struct checks out.
    pop     %bx
    jmp     _use_notpxe_struct

    # The PXENV+ struct should be used.  We need to reload its address.
_use_pxenv_struct:
    lgs     _saved_es_bx, %bx
    movl    %gs:10(%bx), %eax
    movl    %eax, _pxe_api_far_ptr
    lea     .L_use_pxenv_struct_text, %si
    call    _puts
    jmp     _pxe_load_kernel

    # The !PXE struct should be used.  It's address is in GS:BX.
_use_notpxe_struct:
    movl    %gs:16(%bx), %eax
    movl    %eax, _pxe_api_far_ptr
    lea     .L_use_notpxe_struct_text, %si
    call    _puts
    jmp     _pxe_load_kernel

    # Okay, we have an entry point and should try to load the kernel.
_pxe_load_kernel:
    movl    _pxe_api_far_ptr, %eax
    test    %eax, %eax
    je      _pxe_api_null_ptr

    # Okay, tell everyone how we're doing.
    lea     .L_pxe_loading_beginning_text, %si
    call    _puts

    # TODO: PXE fetch lokios.1 (the kernel) to address 2M.
    # For now, we simply write a HLT instruction there.
    mov     $_kernel_entry, %eax
    movb    $0xF4, %fs:(%eax)

    # Jump to the common entry point.
    jmp     _common_entry

    # The PXENV+ struct was bad.  We are just going to bail out back to BIOS at
    # this point.
_pxenv_checksum_mismatch:
    mov     %eax, %edx
    call    _put32
    lea     .L_pxenv_checksum_mismatch_text, %si
    call    _puts
    ret

    # The PXENV+ struct is before 2.1.  This means !PXE isn't supported by this
    # version of the BIOS, so just go with PXENV+.
_pxenv_below_2_1:
    lea     .L_pxenv_below_2_1_text, %si
    call    _puts
    jmp     _use_pxenv_struct

    # The !PXE struct is missing the "!PXE" signature.  Go with the PXENV+
    # struct.
_notpxe_sig_missing:
    lea     .L_notpxe_sig_missing_text, %si
    call    _puts
    jmp     _use_pxenv_struct

    # The !PXE struct has an invalid checksum.  Go with the PXENV+ struct.
_notpxe_checksum_mismatch:
    mov     %eax, %edx
    call    _put32
    lea     .L_notpxe_checksum_mismatch_text, %si
    call    _puts
    jmp     _use_pxenv_struct

    # The PXE API we are using has a NULL pointer!  Return to BIOS.
_pxe_api_null_ptr:
    lea     .L_pxe_api_null_ptr_text, %si
    call    _puts
    ret


# On entry:
#   GS:BX   - address to checksum
#   ECX     - number of bytes to checksum
# On exit:
#   AL      - checksum value
_checksum:
    mov     $0, %eax
.L_checksum_loop:
    addb    %gs:(%bx), %al
    inc     %bx
    loop    .L_checksum_loop
    ret


# Dump memory.
# On entry:
#   GS:BX   - base address of memory
#   ECX     - number of 32-bit words to dump
_dump_mem:
    mov     %bx, %si
    mov     %cx, %di
.L_dump_mem_loop:
    mov     %gs:(%si), %edx
    call    _put32
    mov     $' ', %al
    call    _putc
    add     $4, %si
    dec     %di
    jne     .L_dump_mem_loop
    ret


.data
.L_pxe_bootloader_started_text:
    .asciz  "PXE bootloader started\r\n"
.L_pxenv_below_2_1_text:
    .asciz   "PXENV+ struct is less than 2.1, !PXE not supported"
.L_notpxe_sig_missing_text:
    .asciz   "!PXE signature is missing"
.L_pxenv_checksum_mismatch_text:
    .asciz  " <- PXENV+ struct has invalid checksum\r\n"
.L_notpxe_checksum_mismatch_text:
    .asciz  " <- !PXE struct has invalid checksum\r\n"
.L_use_pxenv_struct_text:
    .asciz  "Using PXENV+ struct\r\n"
.L_use_notpxe_struct_text:
    .asciz  "Using !PXE struct\r\n"
.L_pxe_api_null_ptr_text:
    .asciz  "Chosen PXE API has a NULL real mode API entry pointer!\r\n"
.L_pxe_loading_beginning_text:
    .asciz  "Fetching kernel via PXE...\r\n"
_pxe_api_far_ptr:
    .long   0
