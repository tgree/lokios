SUBMODULES := test

ARP_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
ARP_OBJ := $(ARP_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(ARP_OBJ:.o=.d)

$(LIB_DIR)/arp.a: $(ARP_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(ARP_OBJ)
