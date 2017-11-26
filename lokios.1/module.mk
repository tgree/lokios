CRTBEGIN_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
LSUPCXXX_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=libsupc++.a)
LGCC_EH_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=libgcc_eh.a)

LOKIOS_1_OBJ := \
	lokios.1/entry.o   	\
	lokios.1/init.o	   	\
	lokios.1/crti.o    	\
	lokios.1/main.o    	\
	lokios.1/abort.o   	\
	lokios.1/console.o 	\
	lokios.1/char_stream.o	\
	lokios.1/cxxabi.o       \
	lokios.1/libc.o         \
	lokios.1/tls.o          \
	lokios.1/crtn.o
LOKIOS_1_LINK_OBJ := \
	$(CRTBEGIN_OBJ) \
    	$(LOKIOS_1_OBJ) \
	$(LSUPCXXX_OBJ) \
	$(LGCC_EH_OBJ)  \
	$(CRTEND_OBJ)   \

CLEAN += \
	$(LOKIOS_1_OBJ)	      \
	$(LOKIOS_1_OBJ:.o=.d) \
	lokios.1/lokios.1.elf \
	lokios.1/lokios.1.map

-include $(LOKIOS_1_OBJ:.o=.d)

$(LOKIOS_1_OBJ): ASFLAGS := $(X86_64_ASFLAGS)
$(LOKIOS_1_OBJ): CXXFLAGS := $(X86_64_CXXFLAGS)
lokios.1/main.o: CXXFLAGS := $(X86_64_CXXFLAGS) -Wno-main

lokios.1/lokios.1.elf: $(LOKIOS_1_OBJ) lokios.1/lokios.1.ld lokios.1/module.mk
	ld -melf_x86_64                       \
	   -Map=lokios.1/lokios.1.map         \
	   -T lokios.1/lokios.1.ld            \
	   -o lokios.1/lokios.1.elf           \
	   $(LOKIOS_1_LINK_OBJ)               \
