TESTS := char_stream_test \
	 klist_test       \
	 sort_test        \
	 heap_test        \
	 region_set_test  \
	 vector_test      \
	 kmath_test       \
	 delegate_test    \
	 hash_test        \
	 hash_table_test  \
	 ring_test

char_stream_test.objs := \
	$(MODULE_TBUILD_DIR)/char_stream_test.o \
	$(PARENT_TBUILD_DIR)/char_stream.o \
	$(PARENT_TBUILD_DIR)/kprintf.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o

klist_test.objs := \
    	$(MODULE_TBUILD_DIR)/klist_test.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o

sort_test.objs := \
    	$(MODULE_TBUILD_DIR)/sort_test.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o

heap_test.objs := \
    	$(MODULE_TBUILD_DIR)/heap_test.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o

region_set_test.objs := \
    	$(MODULE_TBUILD_DIR)/region_set_test.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o

vector_test.objs := \
    	$(MODULE_TBUILD_DIR)/vector_test.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fpage.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o

kmath_test.objs := \
    	$(MODULE_TBUILD_DIR)/kmath_test.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o

delegate_test.objs := \
    	$(MODULE_TBUILD_DIR)/delegate_test.o

hash_test.objs := \
    	$(MODULE_TBUILD_DIR)/hash_test.o

hash_table_test.objs := \
    	$(MODULE_TBUILD_DIR)/hash_table_test.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fbuddy_allocator.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fpage.o \
	$(BUILD_TO_DIR)/lokios.1/mm/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o

ring_test.objs := \
    	$(MODULE_TBUILD_DIR)/ring_test.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o
