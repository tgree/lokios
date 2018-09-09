TESTS := json_test

json_test.objs := \
	$(MODULE_TBUILD_DIR)/json_test.o \
	$(PARENT_TBUILD_DIR)/json.o \
	$(BUILD_TO_DIR)/lokios.1/kern/mock/fkassert.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fpage.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fbuddy.o
