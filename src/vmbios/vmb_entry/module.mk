VMB_ENTRY_OBJ := \
	$(MODULE_BUILD_DIR)/0xFFFFFFF0.o

$(VMB_ENTRY_OBJ): ASFLAGS := $(MODE16_ASFLAGS)
$(LIB_DIR)/vmb_entry.a: $(VMB_ENTRY_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(VMB_ENTRY_OBJ)
