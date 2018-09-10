TESTS := \
    	ram_device_test \
	vm_test

ram_device_test.objs := \
    	$(MODULE_TBUILD_DIR)/ram_device_test.o \
	$(PARENT_TBUILD_DIR)/ram_device.o \
	$(BUILD_TO_DIR)/lokios.1/kern/mock/fkassert.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fbuddy.o

vm_test.objs := \
    	$(MODULE_TBUILD_DIR)/vm_test.o \
	$(PARENT_TBUILD_DIR)/vm.o \
	$(PARENT_TBUILD_DIR)/ram_device.o \
	$(BUILD_TO_DIR)/lokios.1/kern/mock/fkassert.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fpage.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fbuddy.o
