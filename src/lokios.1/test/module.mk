TESTS := console_test     \
    	 spinlock_test    \
	 libc_test

console_test.objs := \
    	$(MODULE_TBUILD_DIR)/console_test.o \
	$(PARENT_TBUILD_DIR)/console.o \
	$(PARENT_TBUILD_DIR)/vga.o \
	$(PARENT_TBUILD_DIR)/mm/mock/fmm.o \
	$(PARENT_TBUILD_DIR)/mock/fkassert.o \
	$(PARENT_TBUILD_DIR)/mock/ftime.o

console_test.libs := \
	$(LIB_DIR)/k++.a

spinlock_test.objs := \
    	$(MODULE_TBUILD_DIR)/spinlock_test.o \
	$(PARENT_TBUILD_DIR)/mock/fkassert.o

libc_test.objs := \
    	$(MODULE_TBUILD_DIR)/libc_test.o \
	$(PARENT_TBUILD_DIR)/libc_string.o \
	$(PARENT_TBUILD_DIR)/k++/kprintf.o
