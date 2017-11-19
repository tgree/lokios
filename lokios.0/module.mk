LOKIOS_0_OBJ := \
	lokios.0/lokios.o \
	lokios.0/console.o \
	lokios.0/entry.o
CLEAN += \
	$(LOKIOS_0_OBJ)       \
	lokios.0/lokios.0.elf \
	lokios.0/timestamp.s  \
	lokios.0/timestamp.o

$(LOKIOS_0_OBJ): ASFLAGS := $(I386_16_ASFLAGS)
lokios.0/lokios.0.elf: $(LOKIOS_0_OBJ) lokios.0/lokios.ld lokios.0/module.mk
	@echo '.data' > lokios.0/timestamp.s
	@echo '.globl _BUILD_TIME' >> lokios.0/timestamp.s
	@echo '_BUILD_TIME: .asciz "$(NOW)\\r\\n"' >> lokios.0/timestamp.s
	@$(AS) $(I386_16_ASFLAGS) -o lokios.0/timestamp.o lokios.0/timestamp.s
	ld -melf_i386 -T lokios.0/lokios.ld -o lokios.0/lokios.0.elf $(LOKIOS_0_OBJ) lokios.0/timestamp.o
