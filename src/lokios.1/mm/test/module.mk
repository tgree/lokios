TESTS := page_table_test slab_test buddy_allocator_test

page_table_test.objs := \
    	$(MODULE_TBUILD_DIR)/page_table_test.o \
	$(PARENT_TBUILD_DIR)/page_table.o \
	$(PARENT_TBUILD_DIR)/mock/fpage.o \
	$(PARENT_TBUILD_DIR)/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o

slab_test.objs := \
	$(MODULE_TBUILD_DIR)/slab_test.o \
	$(PARENT_TBUILD_DIR)/mock/fpage.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o

buddy_allocator_test.objs := \
	$(MODULE_TBUILD_DIR)/buddy_allocator_test.o \
	$(PARENT_TBUILD_DIR)/mock/fmm.o \
	$(BUILD_TO_DIR)/lokios.1/mock/fkassert.o
