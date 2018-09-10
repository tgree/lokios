SUBMODULES := test
HEADERS += $(MODULE_SRC_DIR)

VMM_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
VMM_OBJ := $(VMM_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(VMM_OBJ:.o=.d)

$(LIB_DIR)/vmm.a: $(VMM_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(VMM_OBJ)
