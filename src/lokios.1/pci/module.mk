SUBMODULES :=
HEADERS += $(MODULE_SRC_DIR)

PCI_SRC := $(wildcard $(MODULE_SRC_DIR)/*.cc)
PCI_OBJ := $(PCI_SRC:$(MODULE_SRC_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o)

LOKIOS_1_CONSTRUCTORS += pci_unclaimed_driver

-include $(PCI_OBJ:.o=.d)

$(LIB_DIR)/pci.a: $(PCI_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(PCI_OBJ)
