SUBMODULES := mm acpi interrupts pci platform k++ test net

CRTBEGIN_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ   := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
LSUPCXX_OBJ  := $(shell $(CC) $(CFLAGS) -print-file-name=libsupc++.a)
LGCC_EH_OBJ  := $(shell $(CC) $(CFLAGS) -print-file-name=libgcc_eh.a)

LOKIOS_1_CXX_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
LOKIOS_1_ASM_SRC := $(wildcard $(MODULE_SRC_DIR)/*.s)
LOKIOS_1_OBJ := $(LOKIOS_1_CXX_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o) \
                $(LOKIOS_1_ASM_SRC:$(MODULE_SRC_DIR)/%.s=$(MODULE_BUILD_DIR)/%.o)
LOKIOS_1_DRIVERS :=
LOKIOS_1_LIB := mm.a acpi.a interrupts.a pci.a platform.a k++.a \
                $(LOKIOS_1_DRIVERS:%=%.a)

-include $(LOKIOS_1_OBJ:.o=.d)

$(MODULE_BUILD_DIR)/lokios.1.elf: LDM  := $(MODULE_BUILD_DIR)/lokios.1.map
$(MODULE_BUILD_DIR)/lokios.1.elf: LDLD := $(MODULE_SRC_DIR)/lokios.1.ld
$(MODULE_BUILD_DIR)/lokios.1.elf: $(LOKIOS_1_OBJ) $(LOKIOS_1_LIB:%=$(LIB_DIR)/%) $(MODULE_SRC_DIR)/lokios.1.ld $(MODULE_MK)
	@echo Linking $@...
	@ld -melf_x86_64 -Map=$(LDM) -T $(LDLD) $(LOKIOS_1_DRIVERS:%=--require-defined=%_driver) -o $@ \
	    	$(CRTBEGIN_OBJ) \
		$(LOKIOS_1_OBJ) \
		-\(             \
		$(LOKIOS_1_LIB:%=$(LIB_DIR)/%) \
		-\)             \
		$(LSUPCXX_OBJ)  \
		$(LGCC_EH_OBJ)  \
		$(CRTEND_OBJ)
