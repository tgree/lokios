SUBMODULES := test

CRTBEGIN_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ   := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
LSUPCXXX_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=libsupc++.a)
LGCC_EH_OBJ  := $(shell $(CC) $(CFLAGS) -print-file-name=libgcc_eh.a)

LOKIOS_1_OBJ := \
	$(MODULE_BUILD_DIR)/entry.o		\
	$(MODULE_BUILD_DIR)/init.o		\
	$(MODULE_BUILD_DIR)/crti.o		\
	$(MODULE_BUILD_DIR)/main.o		\
	$(MODULE_BUILD_DIR)/kassert.o		\
	$(MODULE_BUILD_DIR)/console.o		\
	$(MODULE_BUILD_DIR)/char_stream.o		\
	$(MODULE_BUILD_DIR)/cxxabi.o		\
	$(MODULE_BUILD_DIR)/libc.o		\
	$(MODULE_BUILD_DIR)/tls.o			\
	$(MODULE_BUILD_DIR)/crtn.o
-include $(LOKIOS_1_OBJ:.o=.d)

LOKIOS_1_LINK_OBJ := \
	$(CRTBEGIN_OBJ)	\
	$(LOKIOS_1_OBJ)	\
	$(LSUPCXXX_OBJ)	\
	$(LGCC_EH_OBJ)	\
	$(CRTEND_OBJ)	\

lokios.1/main.o: CXXFLAGS := $(CXXFLAGS) -Wno-main

$(MODULE_BUILD_DIR)/lokios.1.elf: LDM  := $(MODULE_BUILD_DIR)/lokios.1.map
$(MODULE_BUILD_DIR)/lokios.1.elf: LDLD := $(MODULE_SRC_DIR)/lokios.1.ld
$(MODULE_BUILD_DIR)/lokios.1.elf: $(LOKIOS_1_OBJ) $(MODULE_SRC_DIR)/lokios.1.ld $(MODULE_MK)
	ld -melf_x86_64 -Map=$(LDM) -T $(LDLD) -o $@ $(LOKIOS_1_LINK_OBJ)
