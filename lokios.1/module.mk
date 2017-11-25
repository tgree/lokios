CRTBEGIN_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
LOKIOS_1_OBJ := \
	lokios.1/entry.o   \
	lokios.1/init.o	   \
	lokios.1/crti.o    \
	lokios.1/main.o    \
	lokios.1/console.o \
	lokios.1/crtn.o
LOKIOS_1_LINK_OBJ := \
	$(CRTBEGIN_OBJ) \
    	$(LOKIOS_1_OBJ) \
	$(CRTEND_OBJ)   \

CLEAN += \
	$(LOKIOS_1_OBJ)	      \
	$(LOKIOS_1_OBJ:.o=.d) \
	lokios.1/lokios.1.elf

-include $(LOKIOS_1_OBJ:.o=.d)

$(LOKIOS_1_OBJ): ASFLAGS := $(X86_64_ASFLAGS)
$(LOKIOS_1_OBJ): CXXFLAGS := $(X86_64_CXXFLAGS)
lokios.1/main.o: CXXFLAGS := $(X86_64_CXXFLAGS) -Wno-main

lokios.1/lokios.1.elf: $(LOKIOS_1_OBJ) lokios.1/lokios.1.ld lokios.1/module.mk
	ld -melf_x86_64                       \
	   -T lokios.1/lokios.1.ld            \
	   -o lokios.1/lokios.1.elf           \
	   $(LOKIOS_1_LINK_OBJ)               \
