SUBMODULES := vmb_entry vmb_main

VMBIOS_LIB := vmb_entry.a vmb_main.a

$(MODULE_BUILD_DIR)/vmbios.elf: LDM  := $(MODULE_BUILD_DIR)/vmbios.map
$(MODULE_BUILD_DIR)/vmbios.elf: LDLD := $(MODULE_SRC_DIR)/vmbios.ld
$(MODULE_BUILD_DIR)/vmbios.elf: $(VMBIOS_LIB:%=$(LIB_DIR)/%) $(MODULE_SRC_DIR)/vmbios.ld $(MODULE_MK)
	@echo 'Linking $@...'
	@ld -melf_i386 -Map=$(LDM) -T $(LDLD) -o $@   \
	    -\(                                       \
	    $(VMBIOS_LIB:%=$(LIB_DIR)/%)              \
	    -\)
