TESTS := vga_console_test

vga_console_test.objs := \
    	$(MODULE_TBUILD_DIR)/console_test.o \
	$(PARENT_TBUILD_DIR)/console.o \
	$(PARENT_TBUILD_DIR)/vga.o \
	$(PARENT_TBUILD_DIR)/mm/mock/fmm.o \
	$(PARENT_TBUILD_DIR)/mock/fkassert.o \
	$(PARENT_TBUILD_DIR)/mock/ftime.o

vga_console_test.libs := \
	$(LIB_DIR)/k++.a
