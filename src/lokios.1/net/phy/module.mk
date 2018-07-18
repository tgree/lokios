SUBMODULES :=

NET_PHYS_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
NET_PHYS_OBJ := $(NET_PHYS_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

LOKIOS_1_CONSTRUCTORS += bcm57765_driver

-include $(NET_PHYS_OBJ:.o=.d)

$(LIB_DIR)/net_phys.a: $(NET_PHYS_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(NET_PHYS_OBJ)
