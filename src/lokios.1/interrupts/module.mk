SUBMODULES :=
HEADERS += $(MODULE_SRC_DIR)

INTERRUPTS_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
INTERRUPTS_OBJ := $(INTERRUPTS_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(INTERRUPTS_OBJ:.o=.d)

$(LIB_DIR)/interrupts.a: $(INTERRUPTS_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(INTERRUPTS_OBJ)
