SUBMODULES :=

ETH_PHYS_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
ETH_PHYS_OBJ := $(ETH_PHYS_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

LOKIOS_1_CONSTRUCTORS += bcm57765_driver

-include $(ETH_PHYS_OBJ:.o=.d)

$(LIB_DIR)/eth_phys.a: $(ETH_PHYS_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(ETH_PHYS_OBJ)
