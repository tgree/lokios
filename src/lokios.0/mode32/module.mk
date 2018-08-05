MODE32_CXX_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
MODE32_ASM_SRC := $(wildcard $(MODULE_SRC_DIR)/*.s)

MODE32_OBJ := $(MODE32_CXX_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o) \
              $(MODE32_ASM_SRC:$(MODULE_SRC_DIR)/%.s=$(MODULE_BUILD_DIR)/%.o)

-include $(MODE32_OBJ:.o=.d)

$(MODE32_OBJ): ASFLAGS := $(MODE32_ASFLAGS)
$(MODE32_OBJ): KERN_CXXFLAGS := $(MODE32_CXXFLAGS)
$(LIB_DIR)/mode32.a: $(MODE32_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(MODE32_OBJ)
