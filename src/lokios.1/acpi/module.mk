SUBMODULES := test
HEADERS += $(MODULE_SRC_DIR)

ACPI_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
ACPI_OBJ := $(ACPI_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

-include $(ACPI_OBJ:.o=.d)

$(LIB_DIR)/acpi.a: $(ACPI_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(ACPI_OBJ)
