TESTS := \
    	tcp_header_test \
    	tcp_checksum_test

tcp_header_test.objs := $(MODULE_TBUILD_DIR)/tcp_header_test.o

tcp_checksum_test.objs :=  \
    	$(MODULE_TBUILD_DIR)/tcp_checksum_test.o \
	$(PARENT_TBUILD_DIR)/header.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o
