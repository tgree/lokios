SUBMODULES := phy e1000 virtio bcm57762 test

NET_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
NET_OBJ := $(NET_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(NET_OBJ:.o=.d)

$(LIB_DIR)/net.a: $(NET_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(NET_OBJ)
