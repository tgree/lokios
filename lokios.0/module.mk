LOKIOS_0_OBJ := \
	lokios.0/lokios.o \
	lokios.0/console.o \
	lokios.0/entry.o
CLEAN += \
	$(LOKIOS_0_OBJ)      \
	lokios.0/lokios.elf  \
	lokios.0/timestamp.s \
	lokios.0/timestamp.o

lokios.0/lokios.0.elf: $(LOKIOS_0_OBJ) lokios.0/lokios.ld
	@echo '.data' > lokios.0/timestamp.s
	@echo '.globl _BUILD_TIME' >> lokios.0/timestamp.s
	@echo '_BUILD_TIME: .asciz "$(NOW)"' >> lokios.0/timestamp.s
	@$(AS) $(ASFLAGS) -o lokios.0/timestamp.o lokios.0/timestamp.s
	ld -melf_i386 -T lokios.0/lokios.ld -o lokios.0/lokios.0.elf $(LOKIOS_0_OBJ) lokios.0/timestamp.o
