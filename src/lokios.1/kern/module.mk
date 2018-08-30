SUBMODULES := test
HEADERS += $(MODULE_SRC_DIR)

ENTRY_ASM := $(wildcard $(MODULE_SRC_DIR)/*.s)
ENTRY_OBJ := $(ENTRY_ASM:$(MODULE_SRC_DIR)/%.s=$(MODULE_BUILD_DIR)/%.o)
LOKIOS_1_OBJ += $(ENTRY_OBJ)

KERN_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
KERN_OBJ := $(KERN_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(KERN_OBJ:.o=.d)

$(LIB_DIR)/kern.a: $(ENTRY_OBJ) $(KERN_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(KERN_OBJ)
