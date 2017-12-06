SUBMODULES := test
HEADERS += $(MODULE_SRC_DIR)

KXX_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
KXX_OBJ := $(KXX_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(KXX_OBJ:.o=.d)

$(LIB_DIR)/k++.a: $(KXX_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(KXX_OBJ)
