TESTS := \
    	tcp_header_test \
    	tcp_checksum_test \
	tcp_socket_test

tcp_header_test.objs := \
    	$(MODULE_TBUILD_DIR)/tcp_header_test.o \
	$(BUILD_TO_DIR)/lokios.1/k++/mock/frandom.o

tcp_checksum_test.objs :=  \
    	$(MODULE_TBUILD_DIR)/tcp_checksum_test.o \
	$(PARENT_TBUILD_DIR)/header.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o

tcp_socket_test.objs := \
    	$(MODULE_TBUILD_DIR)/tcp_socket_test.o \
	$(PARENT_TBUILD_DIR)/socket.o \
	$(PARENT_TBUILD_DIR)/tcp.o \
	$(BUILD_TO_DIR)/lokios.1/net/interface.o \
	$(BUILD_TO_DIR)/lokios.1/net/mock/finterface.o \
	$(BUILD_TO_DIR)/lokios.1/net/mock/mcmd_sock.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fschedule.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fconsole.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fpage.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fbuddy_allocator.o \
	$(BUILD_TO_DIR)/lokios.1/k++/mock/frandom.o
