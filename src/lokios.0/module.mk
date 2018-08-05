SUBMODULES := bootsector

LOKIOS_0_CXX_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
LOKIOS_0_ASM_SRC := $(wildcard $(MODULE_SRC_DIR)/*.s)

LOKIOS_0_OBJ := $(LOKIOS_0_CXX_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o) \
                $(LOKIOS_0_ASM_SRC:$(MODULE_SRC_DIR)/%.s=$(MODULE_BUILD_DIR)/%.o)

$(LOKIOS_0_OBJ): ASFLAGS := $(MODE16_ASFLAGS)
$(LOKIOS_0_OBJ): KERN_CXXFLAGS := $(MODE16_CXXFLAGS)

LOKIOS_0_LIB := bootsector.a

TIMESTAMP_S := $(MODULE_BUILD_DIR)/timestamp.s
TIMESTAMP_O := $(MODULE_BUILD_DIR)/timestamp.o

-include $(LOKIOS_0_OBJ:.o=.d)

$(MODULE_BUILD_DIR)/lokios.0.elf: LDM  := $(MODULE_BUILD_DIR)/lokios.0.map
$(MODULE_BUILD_DIR)/lokios.0.elf: LDLD := $(MODULE_SRC_DIR)/lokios.0.ld
$(MODULE_BUILD_DIR)/lokios.0.elf: $(LOKIOS_0_OBJ) $(LOKIOS_0_LIB:%=$(LIB_DIR)/%) $(MODULE_SRC_DIR)/lokios.0.ld $(MODULE_MK)
	@echo '.data' > $(TIMESTAMP_S)
	@echo '.globl _BUILD_TIME' >> $(TIMESTAMP_S)
	@echo '_BUILD_TIME: .asciz "$(NOW)\\r\\n"' >> $(TIMESTAMP_S)
	@$(AS) $(MODE16_ASFLAGS) -o $(TIMESTAMP_O) $(TIMESTAMP_S)
	@echo 'Linking $@...'
	@ld -melf_i386 -Map=$(LDM) -T $(LDLD) --whole-archive -o $@ \
	    $(LOKIOS_0_OBJ)                                         \
	    $(TIMESTAMP_O)                                          \
	    -\(                                                     \
	    $(LOKIOS_0_LIB:%=$(LIB_DIR)/%)                          \
	    -\)                                                     \
