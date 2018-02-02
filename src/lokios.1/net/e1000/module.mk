SUBMODULES :=
HEADERS +=

E1000_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
E1000_OBJ := $(E1000_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(E1000_OBJ:.o=.d)

$(LIB_DIR)/e1000.a: $(E1000_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(E1000_OBJ)
