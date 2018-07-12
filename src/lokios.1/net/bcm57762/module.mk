SUBMODULES :=
HEADERS +=

BCM57762_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
BCM57762_OBJ := $(BCM57762_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(BCM57762_OBJ:.o=.d)

$(LIB_DIR)/bcm57762.a: $(BCM57762_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(BCM57762_OBJ)
