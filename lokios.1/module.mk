LOKIOS_1_OBJ := \
	lokios.1/entry.o
CLEAN += \
	$(LOKIOS_1_OBJ)	      \
	lokios.1/lokios.1.elf

$(LOKIOS_1_OBJ): ASFLAGS := $(X86_64_ASFLAGS)
lokios.1/lokios.1.elf: $(LOKIOS_1_OBJ) lokios.1/lokios.1.ld lokios.1/module.mk
	ld -melf_x86_64 -T lokios.1/lokios.1.ld -o lokios.1/lokios.1.elf $(LOKIOS_1_OBJ)
