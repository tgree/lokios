SUBMODULES := test

DHCP_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
DHCP_OBJ := $(DHCP_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(DHCP_OBJ:.o=.d)

$(LIB_DIR)/dhcp.a: $(DHCP_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(DHCP_OBJ)
