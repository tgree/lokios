TESTS := dhcpc_test

dhcpc_test.objs := \
    	$(MODULE_TBUILD_DIR)/dhcpc_test.o \
	$(PARENT_TBUILD_DIR)/dhcpc.o \
	$(PARENT_TBUILD_DIR)/dhcp.o \
	$(BUILD_TO_DIR)/lokios.1/net/mock/minterface.o \
	$(BUILD_TO_DIR)/lokios.1/net/tcp/mock/msocket.o \
	$(BUILD_TO_DIR)/lokios.1/net/eth/mock/minterface.o \
	$(BUILD_TO_DIR)/lokios.1/wapi/mock/mwapi.o \
	$(BUILD_TO_DIR)/lokios.1/k++/mock/frandom.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fpage.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fbuddy_allocator.o \
	$(BUILD_TO_DIR)/lokios.1/kern/mock/fkassert.o \
	$(BUILD_TO_DIR)/lokios.1/kern/mock/fconsole.o \
	$(BUILD_TO_DIR)/lokios.1/kern/mock/fschedule.o
