SUBMODULES :=
HEADERS += $(MODULE_SRC_DIR)

WAPI_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
WAPI_OBJ := $(WAPI_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

LOKIOS_1_CONSTRUCTORS += wapi_observer

-include $(WAPI_OBJ:.o=.d)

$(LIB_DIR)/wapi.a: $(WAPI_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(WAPI_OBJ)
