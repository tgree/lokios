TESTS := spinlock_test \
         kstring_test

spinlock_test.objs := \
    	$(MODULE_TBUILD_DIR)/spinlock_test.o \
	$(PARENT_TBUILD_DIR)/mock/fkassert.o

kstring_test.objs := \
	$(MODULE_TBUILD_DIR)/kstring_test.o \
	$(BUILD_TO_DIR)/lokios.1/k++/char_stream.o \
	$(BUILD_TO_DIR)/lokios.1/k++/kprintf.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fbuddy.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/kern/mock/fkassert.o
