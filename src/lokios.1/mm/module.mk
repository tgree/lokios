SUBMODULES := test
HEADERS += $(MODULE_SRC_DIR)

MM_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
MM_OBJ := $(MM_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(MM_OBJ:.o=.d)

$(LIB_DIR)/mm.a: $(MM_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(MM_OBJ)
