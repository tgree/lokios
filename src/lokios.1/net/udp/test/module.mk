TESTS := udp_csum_test

udp_csum_test.objs := \
	$(MODULE_TBUILD_DIR)/udp_csum_test.o \
	$(PARENT_TBUILD_DIR)/udp.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o
