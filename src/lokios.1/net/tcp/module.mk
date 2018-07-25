SUBMODULES := test

TCP_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
TCP_OBJ := $(TCP_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(TCP_OBJ:.o=.d)

$(LIB_DIR)/tcp.a: $(TCP_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(TCP_OBJ)
