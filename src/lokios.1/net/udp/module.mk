SUBMODULES := test

UDP_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
UDP_OBJ := $(UDP_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(UDP_OBJ:.o=.d)

$(LIB_DIR)/udp.a: $(UDP_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(UDP_OBJ)
