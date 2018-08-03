BOOTSECTOR_OBJ := \
	$(MODULE_BUILD_DIR)/disk.o    \
	$(MODULE_BUILD_DIR)/console.o \
	$(MODULE_BUILD_DIR)/entry.o

$(BOOTSECTOR_OBJ): ASFLAGS := $(MODE16_ASFLAGS)
$(MODULE_BUILD_DIR)/bootsector.a: $(BOOTSECTOR_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@$(AR) $(ARFLAGS) $@ $(BOOTSECTOR_OBJ)
