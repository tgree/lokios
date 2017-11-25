LOKIOS_1_OBJ := \
	lokios.1/entry.o \
	lokios.1/main.o
CLEAN += \
	$(LOKIOS_1_OBJ)	      \
	lokios.1/lokios.1.elf

$(LOKIOS_1_OBJ): ASFLAGS := $(X86_64_ASFLAGS)
$(LOKIOS_1_OBJ): CXXFLAGS := $(X86_64_CXXFLAGS)
lokios.1/main.o: CXXFLAGS := $(X86_64_CXXFLAGS) -Wno-main

lokios.1/lokios.1.elf: $(LOKIOS_1_OBJ) lokios.1/lokios.1.ld lokios.1/module.mk
	ld -melf_x86_64 -T lokios.1/lokios.1.ld -o lokios.1/lokios.1.elf $(LOKIOS_1_OBJ)
