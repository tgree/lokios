TESTS := page_table_test slab_test

page_table_test.objs := \
    	$(MODULE_TBUILD_DIR)/page_table_test.o \
	$(PARENT_TBUILD_DIR)/page_table.o

slab_test.objs := \
	$(MODULE_TBUILD_DIR)/slab_test.o
