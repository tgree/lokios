TESTS := console_test     \

console_test.objs := \
    	$(MODULE_BUILD_DIR)/console_test.o \
	$(PARENT_BUILD_DIR)/console.o \

console_test.libs := \
	$(LIB_DIR)/k++.a
