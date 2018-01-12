TESTS := console_test     \

console_test.objs := \
    	$(MODULE_TBUILD_DIR)/console_test.o \
	$(PARENT_TBUILD_DIR)/console.o \
	$(PARENT_TBUILD_DIR)/vga.o \

console_test.libs := \
	$(LIB_DIR)/k++.a
