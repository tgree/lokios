LOKIOS_1_OBJ := \
	lokios.1/entry.o       \
	lokios.1/e820.o        \
	lokios.1/a20_gate.o    \
	lokios.1/unreal_mode.o \
	lokios.1/long_mode.o   \
	lokios.1/start_c.o
CLEAN += \
	$(LOKIOS_1_OBJ)             \
	lokios.1/lokios.1.elf

$(LOKIOS_1_OBJ): ASFLAGS := $(I386_16_ASFLAGS)
$(LOKIOS_1_OBJ): CFLAGS := $(I386_32_CFLAGS)
lokios.1/lokios.1.elf: $(LOKIOS_1_OBJ) lokios.1/lokios.1.ld
	ld -melf_i386 -T lokios.1/lokios.1.ld -o lokios.1/lokios.1.elf $(LOKIOS_1_OBJ)
