LOKIOS_1_OBJ := \
	lokios.1/entry.o    \
	lokios.1/e820.o     \
	lokios.1/a20_gate.o
LOKIOS_1_OBJ_64 :=
CLEAN += \
	$(LOKIOS_1_OBJ)             \
	$(LOKIOS_1_OBJ_64)          \
	lokios.1/lokios.1.16.elf    \
	lokios.1/lokios.1.16.64.elf \
	lokios.1/lokios.1.elf

$(LOKIOS_1_OBJ): ASFLAGS := $(I386_16_ASFLAGS)
lokios.1/lokios.1.elf: $(LOKIOS_1_OBJ) $(LOKIOS_1_OBJ_64) lokios.1/lokios.1.16.ld lokios.1/lokios.1.ld
	ld -i -melf_i386 -T lokios.1/lokios.1.16.ld -o lokios.1/lokios.1.16.elf $(LOKIOS_1_OBJ)
	objcopy -I elf32-i386 -O elf64-x86-64 lokios.1/lokios.1.16.elf lokios.1/lokios.1.16.64.elf
	ld -melf_x86_64 -T lokios.1/lokios.1.ld -o lokios.1/lokios.1.elf lokios.1/lokios.1.16.64.elf $(LOKIOS_1_OBJ_64)
