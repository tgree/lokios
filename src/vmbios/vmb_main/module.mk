VMB_MAIN_OBJ := \
	$(MODULE_BUILD_DIR)/vmb_main.o

$(VMB_MAIN_OBJ): ASFLAGS := $(MODE16_ASFLAGS)
$(LIB_DIR)/vmb_main.a: $(VMB_MAIN_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(VMB_MAIN_OBJ)
