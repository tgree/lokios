TESTS := dhcpc_test

dhcpc_test.objs := \
    	$(MODULE_TBUILD_DIR)/dhcpc_test.o \
	$(PARENT_TBUILD_DIR)/dhcpc.o \
	$(PARENT_TBUILD_DIR)/dhcp.o \
	$(BUILD_TO_DIR)/lokios.1/net/eth/mock/meth.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fpage.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fconsole.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fschedule.o
