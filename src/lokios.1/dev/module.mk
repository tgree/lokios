SUBMODULES :=
HEADERS += $(MODULE_SRC_DIR)

DEV_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
DEV_OBJ := $(DEV_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(DEV_OBJ:.o=.d)

$(LIB_DIR)/dev.a: $(DEV_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(DEV_OBJ)
