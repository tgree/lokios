TESTS := page_table_test slab_test buddy_allocator_test

page_table_test.objs := \
    	$(MODULE_TBUILD_DIR)/page_table_test.o \
	$(PARENT_TBUILD_DIR)/page_table.o

slab_test.objs := \
	$(MODULE_TBUILD_DIR)/slab_test.o

buddy_allocator_test.objs := \
	$(MODULE_TBUILD_DIR)/buddy_allocator_test.o
