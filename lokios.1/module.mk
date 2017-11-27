CRTBEGIN_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ   := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
LSUPCXXX_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=libsupc++.a)
LGCC_EH_OBJ  := $(shell $(CC) $(CFLAGS) -print-file-name=libgcc_eh.a)

LOKIOS_1_OBJ := \
	$(MODULE)/entry.o		\
	$(MODULE)/init.o		\
	$(MODULE)/crti.o		\
	$(MODULE)/main.o		\
	$(MODULE)/kassert.o		\
	$(MODULE)/console.o		\
	$(MODULE)/char_stream.o		\
	$(MODULE)/cxxabi.o		\
	$(MODULE)/libc.o		\
	$(MODULE)/tls.o			\
	$(MODULE)/crtn.o
LOKIOS_1_LINK_OBJ := \
	$(CRTBEGIN_OBJ)	\
	$(LOKIOS_1_OBJ)	\
	$(LSUPCXXX_OBJ)	\
	$(LGCC_EH_OBJ)	\
	$(CRTEND_OBJ)	\

CLEAN += \
	$(LOKIOS_1_OBJ)		\
	$(LOKIOS_1_OBJ:.o=.d)	\
	$(MODULE)/lokios.1.elf	\
	$(LD_MAP_TARGET)

-include $(LOKIOS_1_OBJ:.o=.d)

$(LOKIOS_1_OBJ): ASFLAGS := $(X86_64_ASFLAGS)
lokios.1/main.o: CXXFLAGS := $(CXXFLAGS) -Wno-main

$(MODULE)/lokios.1.elf: LDM  := $(MODULE)/lokios.1.map
$(MODULE)/lokios.1.elf: LDLD := $(MODULE)/lokios.1.ld
$(MODULE)/lokios.1.elf: $(LOKIOS_1_OBJ) $(MODULE)/lokios.1.ld $(MODULE)/module.mk
	ld -melf_x86_64 -Map=$(LDM) -T $(LDLD) -o $@ $(LOKIOS_1_LINK_OBJ)
