SUBMODULES :=
HEADERS += $(MODULE_SRC_DIR)

CMD_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
CMD_OBJ := $(CMD_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

LOKIOS_1_CONSTRUCTORS += cmd_sock_observer

-include $(CMD_OBJ:.o=.d)

$(LIB_DIR)/cmd.a: $(CMD_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(CMD_OBJ)
