SUBMODULES :=
HEADERS += $(MODULE_SRC_DIR)

PLATFORM_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
PLATFORM_OBJ := $(PLATFORM_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(PLATFORM_OBJ:.o=.d)

$(LIB_DIR)/platform.a: $(PLATFORM_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(PLATFORM_OBJ)
