TESTS := arp_test dhcpc_test

arp_test.objs := \
    	$(MODULE_TBUILD_DIR)/arp_test.o

dhcpc_test.objs := \
    	$(MODULE_TBUILD_DIR)/dhcpc_test.o \
	$(PARENT_TBUILD_DIR)/dhcpc.o \
	$(PARENT_TBUILD_DIR)/dhcp.o \
	$(PARENT_TBUILD_DIR)/mock/meth.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fpage.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fconsole.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fschedule.o
