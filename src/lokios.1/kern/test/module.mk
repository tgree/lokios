TESTS := spinlock_test

spinlock_test.objs := \
    	$(MODULE_TBUILD_DIR)/spinlock_test.o \
	$(PARENT_TBUILD_DIR)/mock/fkassert.o
