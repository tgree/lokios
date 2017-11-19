LOKIOS_1_OBJ := \
	lokios.1/entry.o \
	lokios.1/e820.o
CLEAN += \
	$(LOKIOS_1_OBJ) \
	lokios.1/lokios.1.elf

$(LOKIOS_1_OBJ): ASFLAGS := $(I386_16_ASFLAGS)
lokios.1/lokios.1.elf: $(LOKIOS_1_OBJ) lokios.1/lokios.1.ld
	ld -melf_i386 -T lokios.1/lokios.1.ld -o lokios.1/lokios.1.elf $(LOKIOS_1_OBJ)
