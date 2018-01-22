TESTS := console_test     \
    	 spinlock_test    \

console_test.objs := \
    	$(MODULE_TBUILD_DIR)/console_test.o \
	$(PARENT_TBUILD_DIR)/console.o \
	$(PARENT_TBUILD_DIR)/vga.o \

console_test.libs := \
	$(LIB_DIR)/k++.a

spinlock_test.objs := \
    	$(MODULE_TBUILD_DIR)/spinlock_test.o
