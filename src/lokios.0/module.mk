SUBMODULES := bootsector mode32

LOKIOS_0_LIB := bootsector.a mode32.a

TIMESTAMP_S := $(MODULE_BUILD_DIR)/timestamp.s
TIMESTAMP_O := $(MODULE_BUILD_DIR)/timestamp.o

$(MODULE_BUILD_DIR)/lokios.0.elf: LDM  := $(MODULE_BUILD_DIR)/lokios.0.map
$(MODULE_BUILD_DIR)/lokios.0.elf: LDLD := $(MODULE_SRC_DIR)/lokios.0.ld
$(MODULE_BUILD_DIR)/lokios.0.elf: $(LOKIOS_0_LIB:%=$(LIB_DIR)/%) $(MODULE_SRC_DIR)/lokios.0.ld $(MODULE_MK)
	@echo '.data' > $(TIMESTAMP_S)
	@echo '.globl _BUILD_BANNER' >> $(TIMESTAMP_S)
	@echo '_BUILD_BANNER: .ascii "Loki\\r\\nCopyright (c) 2017-2018 by Terry Greeniaus.\\r\\n"' >> $(TIMESTAMP_S)
	@echo '.globl _BUILD_TIME' >> $(TIMESTAMP_S)
	@echo '_BUILD_TIME: .asciz "$(NOW)\\r\\n"' >> $(TIMESTAMP_S)
	@$(AS) $(MODE16_ASFLAGS) -o $(TIMESTAMP_O) $(TIMESTAMP_S)
	@echo 'Linking $@...'
	@ld -melf_i386 -Map=$(LDM) -T $(LDLD) --whole-archive -o $@ \
	    $(TIMESTAMP_O)                                          \
	    -\(                                                     \
	    $(LOKIOS_0_LIB:%=$(LIB_DIR)/%)                          \
	    -\)                                                     \
