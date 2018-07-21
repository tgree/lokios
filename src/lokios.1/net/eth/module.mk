SUBMODULES := phy e1000 virtio bcm57762

ETH_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
ETH_OBJ := $(ETH_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(ETH_OBJ:.o=.d)

$(LIB_DIR)/eth.a: $(ETH_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(ETH_OBJ)
