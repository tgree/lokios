.text

# We're in 32-bit protected mode and need to switch to 16-bit real mode.
# Stomps: nothing
.global _mode_switch_real_and_write_dot_code16_after_this_line
_mode_switch_real_and_write_dot_code16_after_this_line:
.code32
    push    %ebx

    # Switch to a 16-bit code segment.
    ljmp    $32, $.L_into_16_mode
.L_into_16_mode:
.code16

    # Switch all segment registers to 16-bit data segments.
    mov     $24, %bx
    mov     %bx, %es
    mov     %bx, %fs
    mov     %bx, %gs
    mov     %bx, %ss
    mov     %bx, %ds

    # Disable protected mode.
    mov     %cr0, %ebx
    and     $~(1<<0), %ebx
    mov     %ebx, %cr0

    # Flush the instruction queue.
    jmp     .L_next
.L_next:

    # Clear CS and all the data segment registers.
    ljmp    $0, $.L_clear_cs
.L_clear_cs:
    mov     $0, %bx
    mov     %bx, %es
    mov     %bx, %fs
    mov     %bx, %gs
    mov     %bx, %ss
    mov     %bx, %ds

    # Manually return via jmp.
    movl    0(%esp), %ebx
    add     $8, %esp
    jmp     *-4(%esp)


# We're in 16-bit real mode and need to switch to 32-bit protected mode.
# Stomps: nothing
.global _mode_switch_protected_and_write_dot_code32_after_this_line
_mode_switch_protected_and_write_dot_code32_after_this_line:
.code16
    pushl   %ebx

    # Enable protected mode.
    mov     %cr0, %ebx
    or      $(1<<0), %ebx
    mov     %ebx, %cr0

    # Flush the instruction queue.
    jmp     .L_next2
.L_next2:

    # Switch to a 32-bit code segment.
    ljmp    $16, $.L_into_32_mode
.L_into_32_mode:
.code32

    # Switch all segment registers to 32-bit data segments.
    mov     $8, %bx
    mov     %bx, %es
    mov     %bx, %fs
    mov     %bx, %gs
    mov     %bx, %ss
    mov     %bx, %ds

    # Manually return via jmp.
    movl    0(%esp), %ebx
    add     $6, %esp
    jmpw    *-2(%esp)


# Jump into long mode from 32-bit protected mode.
# On entry:
#   0(esp)  - return address
#   4(esp)  - 32-bit cr3 value
#   8(esp)  - 64-bit jump target address
#  16(esp)  - 64-bit jump stack address
# On exit:
#   DOES NOT RETURN.
.global _m32_long_jump
_m32_long_jump:
.code32
    cli

    # Load an empty interrupt descriptor table.
    lidt    _idt_64_desc

    # Read parameters off the stack now so that we won't perform unnecessary
    # memory accesses later.
    mov     4(%esp), %ebx

    # Set the PAT MSR to: 
    #   WB WT WC UC WB WT WC UC
    # Note that this makes the PTE PAT bit into a "don't care" bit.
    mov     $0x277, %ecx
    mov     $0x00010406, %edx
    mov     $0x00010406, %eax
    wrmsr

    # Enable PAE.
    mov     %cr4, %eax
    or      $(1<<5), %eax
    mov     %eax, %cr4

    # Set CR3 to point at the first level page table we defined in the linker
    # file.  This page table identity-maps the first 8M of RAM.
    mov     %ebx, %cr3

    # Set EFER.LME and EFER.NXE.
    mov     $0xC0000080, %ecx
    rdmsr
    or      $(1<<8) | (1 << 11), %eax
    wrmsr

    # Enable paging and protected mode.
    mov     %cr0, %eax
    or      $(1<<31) | (1<<0), %eax
    mov     %eax, %cr0

    # OK, we are now in compatibility mode.  We need to load the GDT and use it
    # to jump into our code segment.  The $8 below is the offset to the code
    # descriptor in the GDT.
    ljmp    $48, $.L_into_long_mode
.L_into_long_mode:
.code64

    # Enable write protection.  It seems we have to do this after the long jump
    # above or things break.
    mov     %cr0, %rax
    or      $(1<<16), %rax
    mov     %rax, %cr0

    # Load the branch address and switch to the new stack.
    movq    8(%rsp), %r8
    movq    16(%rsp), %rsp

    # Allocate some space at the top of the stack for a temporary TLS area
    # until kernel_task gets spawned.  We move this into MSR_FS_BASE and
    # MSR_GS_BASE.  We also store the TLS address at offset 0 in the TLS space
    # since TLS addresses use that to convert to absolute addresses.
    mov     $0xC0000100, %ecx
    sub     $64, %rsp
    mov     %rsp, %rax
    mov     %rax, %rdx
    shr     $32, %rdx
    wrmsr
    mov     $0xC0000101, %ecx
    wrmsr
    mov     %rax, 0(%rax)

    # Initialize the stack guard.  Since we are building with -mcmodel-kernel,
    # the stack stomp protecter looks for the stack guard value in %gs:0x28
    # instead of %fs:0x28 like it does for userspace programs.
    mov     $0xA1B2C3D4E5F60718, %rax
    mov     %rax, %gs:0x28

    # Jump to the target address.
    jmp     *%r8


# Call into PXE from protected mode.
# On entry:
#   0(esp)  - return address
#   4(esp)  - opcode
#   8(esp)  - param block pointer
#   12(esp) - pxe far pointer
# On exit:
#   AX    - error code
.globl _call_pxe
_call_pxe:
.code32
    push    %ebx
    push    %edi

    call    _mode_switch_real_and_write_dot_code16_after_this_line
.code16

    xor     %ebx, %ebx
    mov     12(%esp), %bx
    mov     16(%esp), %di
    push    %ds
    push    %di
    push    %bx
    lcall   *26(%esp)
    add     $6, %esp

    call    _mode_switch_protected_and_write_dot_code32_after_this_line
.code32

    pop     %edi
    pop     %ebx
    ret


# Call INT 15h to enable the A20 line from protected mode.
# On entry:
# On exit:
.global _a20_enable_int15h
_a20_enable_int15h:
.code32
    call    _mode_switch_real_and_write_dot_code16_after_this_line
.code16

    movw    $0x2401, %ax
    int     $0x15

    call    _mode_switch_protected_and_write_dot_code32_after_this_line
.code32

    ret


# Call INT 15h to get the E820 memory map.
# On entry:
#   0(sp)   - return address
#   4(sp)   - address of e820_io struct
# On exit:
#   e820_io struct will be updated and ebx should be consulted to see if we
#   are at the end - we massage CF into there for you.
.global e820_iter
e820_iter:
.code32
    push    %ebx

    call    _mode_switch_real_and_write_dot_code16_after_this_line
.code16

    mov     8(%esp), %edi
    mov     $0x0000E820, %eax
    mov     4(%edi), %ebx
    mov     $24, %ecx
    mov     $0x534D4150, %edx
    lea     12(%edi), %edi
    int     $0x15

    mov     8(%esp), %edi
    mov     %eax, 0(%edi)
    mov     %ebx, 4(%edi)
    jnc     .L_E820_no_carry
    movl    $0, 4(%edi)
.L_E820_no_carry:
    xor     %eax, %eax
    mov     %cl, %al
    mov     %eax, 8(%edi)
    
    call    _mode_switch_protected_and_write_dot_code32_after_this_line
.code32

    pop     %ebx

    ret

# Call INT 10h to write a character to the VGA console from protected mode.
# On entry:
#   AX - character to write
.global _vga_putc
_vga_putc:
.code32
    push    %ebx

    call    _mode_switch_real_and_write_dot_code16_after_this_line
.code16

    or      $0x0E00, %ax
    mov     $7, %bx
    int     $0x10

    call    _mode_switch_protected_and_write_dot_code32_after_this_line
.code32

    pop     %ebx
    ret


# Call INT 13h to read some LBAs from disk.
# Uses cdecl calling convention.
.global _m32_disk_read
_m32_disk_read:
.code32
    push    %esi

    call    _mode_switch_real_and_write_dot_code16_after_this_line
.code16

    mov     8(%esp), %edx
    mov     12(%esp), %esi
    mov     $0x4200, %eax
    int     $0x13
    cli
    jc      .L_disk_read_failed
    mov     $0, %eax
    jmp     .L_disk_read_out
.L_disk_read_failed:
    mov     $-1, %eax
.L_disk_read_out:

    call    _mode_switch_protected_and_write_dot_code32_after_this_line
.code32

    pop     %esi
    ret


# Called from the bootsector.
# On entry:
#   EAX - flags value to pass to m32_entry().
.global _m32_entry
_m32_entry:
.code16
    push    %eax
    bt      $0, %eax     
    jc      .L_pxe_entry

.L_mbr_entry:
    lea     .L_mbr_bootloader_started_text, %si
    jmp     .L_jump_to_m3_entry

.L_pxe_entry:
    lea     .L_pxe_bootloader_started_text, %si

.L_jump_to_m3_entry:
    call    _puts
    pop     %eax
    call    _mode_switch_protected_and_write_dot_code32_after_this_line
.code32
    call    m32_entry
    call    _mode_switch_real_and_write_dot_code16_after_this_line
.code16
    ret


.data
.L_mbr_bootloader_started_text:
    .asciz  "MBR bootloader started\r\n"
.L_pxe_bootloader_started_text:
    .asciz  "PXE bootloader started\r\n"
