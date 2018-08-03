BOOTSECTOR_OBJ := \
	$(MODULE_BUILD_DIR)/disk.o    \
	$(MODULE_BUILD_DIR)/console.o \
	$(MODULE_BUILD_DIR)/entry.o

$(BOOTSECTOR_OBJ): ASFLAGS := $(MODE16_ASFLAGS)
$(LIB_DIR)/bootsector.a: $(BOOTSECTOR_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(BOOTSECTOR_OBJ)
