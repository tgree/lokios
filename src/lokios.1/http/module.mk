SUBMODULES :=
HEADERS += $(MODULE_SRC_DIR)

HTTP_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
HTTP_OBJ := $(HTTP_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(HTTP_OBJ:.o=.d)

$(LIB_DIR)/http.a: $(HTTP_OBJ) $(MODULE_MK)
	@echo Archive $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(HTTP_OBJ)
