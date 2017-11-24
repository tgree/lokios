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
    jmp     _pxe_entry_found

    # The !PXE struct should be used.  It's address is in GS:BX.
_use_notpxe_struct:
    movl    %gs:16(%bx), %eax
    movl    %eax, _pxe_api_far_ptr
    lea     .L_use_notpxe_struct_text, %si
    call    _puts
    jmp     _pxe_entry_found

    # Okay, we have found an entry point.  Check it for validity, and if it is
    # good then jump to the code that will load the kernel.
_pxe_entry_found:
    movl    _pxe_api_far_ptr, %eax
    test    %eax, %eax
    je      _pxe_api_null_ptr
    jmp     _pxe_start_load

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


# Ready to start loading the kernel via PXE.
# On entry:
#   nothing.  
_pxe_start_load:
    # Okay, tell everyone how we're doing.
    lea     .L_pxe_loading_beginning_text, %si
    call    _puts

    # Start by getting the cached DHCP response we got from the server.  This
    # is going to tell us the server's IP address.
    lea     _pxe_get_cached_info_cmd, %di
    mov     $0x0071, %bx
    call    _call_pxe
    test    %ax, %ax
    jne     _pxe_get_cached_info_failed
    movw    _pxe_get_cached_info_cmd, %bx
    test    %bx, %bx
    jne     _pxe_get_cached_info_failed

    # Okay, the result was filled in!
    lea     .L_pxe_server_ip_text, %si
    call    _puts
    lgs     _pxe_get_cached_info_cmd + 6, %si
    movl    %gs:20(%si), %edx
    call    _put8
    mov     $'.', %al
    call    _putc
    ror     $8, %edx
    call    _put8
    mov     $'.', %al
    call    _putc
    ror     $8, %edx
    call    _put8
    mov     $'.', %al
    call    _putc
    ror     $8, %edx
    call    _put8
    ror     $8, %edx
    call    _putCRLF

    # Do a TFTP OPEN command for our target file.
    lea     _pxe_open_cmd, %di
    mov     $0x0020, %bx
    movl    %edx, _pxe_open_cmd_server_ip
    call    _call_pxe
    test    %ax, %ax
    jne     _pxe_open_failed
    movw    _pxe_open_cmd, %bx
    test    %bx, %bx
    jne     _pxe_open_failed

    # Do a TFTP READ to get the first 512-byte packet.
    lea     _pxe_read_cmd, %di
    mov     $0x0022, %bx
    call    _call_pxe
    test    %ax, %ax
    jne     _pxe_read_failed
    movw    _pxe_read_cmd, %bx
    test    %bx, %bx
    jne     _pxe_read_failed

    # Copy the packet up into place.  Sadly, calling into PXE breaks unreal
    # mode.  It probably switches into protected mode since it has a protected
    # mode entry point as well, and switching would cause the CPU to then
    # update the FS limit values if PXE changed FS for some reason.
    call    _enter_unreal_mode
    lea     _pre_e820_bounce_buffer, %esi
    mov     $_kernel_base, %edi
    mov     $512/4, %ecx
    call    _unreal_memcpy

    # Up next: loop until we get the full file, do a TFTP_CLOSE.
    # The best strategy would be to do something like what MBR does where it
    # reads 4K worth of sectors and then unreal_memcpys them at once.  That
    # would minimize the number of protected-mode changes - although maybe we
    # don't even care since the PXE API is already switching modes on us so
    # what's another switch really gonna do?

    # Jump to the common entry point.
    jmp     _common_entry

    # Our attempt to read the cached DHCP failed.  Life sucks.
_pxe_get_cached_info_failed:
    mov     %ax, %dx
    call    _put16
    mov     $' ', %al
    call    _putc
    movw    _pxe_get_cached_info_cmd, %dx
    call    _put16
    mov     $' ', %al
    call    _putc
    lea     .L_pxe_get_cached_info_failed_text, %si
    call    _puts
    ret

    # Our TFTP open attempt failed.  Life sucks.
_pxe_open_failed:
    mov     %ax, %dx
    call    _put16
    mov     $' ', %al
    call    _putc
    movw    _pxe_open_cmd, %dx
    call    _put16
    mov     $' ', %al
    call    _putc
    lea     .L_pxe_open_failed_text, %si
    call    _puts
    ret

    # Our TFTP read attempt failed.  Life still sucks.
_pxe_read_failed:
    mov     %ax, %dx
    call    _put16
    mov     $' ', %al
    call    _putc
    movw    _pxe_read_cmd, %dx
    call    _put16
    mov     $' ', %al
    call    _putc
    lea     .L_pxe_read_failed_text, %si
    call    _puts
    ret

# On entry:
#   ES:DI - address of the command buffer
#   BX    - opcode
_call_pxe:
    push    %ds
    push    %di
    push    %bx

    lcall   *_pxe_api_far_ptr

    add     $6, %sp
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
.L_pxe_get_cached_info_failed_text:
    .asciz  "PXE Get Cached Info call failed!\r\n"
.L_pxe_open_failed_text:
    .asciz  "PXE Open call failed!\r\n"
.L_pxe_read_failed_text:
    .asciz  "PXE Read call failed!\r\n"
.L_pxe_server_ip_text:
    .asciz  "PXE server IP: "
_pxe_api_far_ptr:
    .long   0
_pxe_get_cached_info_cmd:
    .short  0
    .short  2   # PXENV_PACKET_TYPE_DHCP_ACK
    .short  0
    .long   0
    .short  0

_pxe_open_cmd:
    .short  0
_pxe_open_cmd_server_ip:
    .long   0   # Server IP Address
    .long   0   # Gateway IP Address
_pxe_open_cmd_filename:
    .asciz  "lokios.1"
    .zero   (128 + _pxe_open_cmd_filename - .)
    .short  (69 << 8) | (0x69 >> 8)  # UDP Port, big-endian!
    .short  512 # Packet size

_pxe_read_cmd:
    .short  0
    .short  0
    .short  0
    .short  _pre_e820_bounce_buffer
    .short  0
