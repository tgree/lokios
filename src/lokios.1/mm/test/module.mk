TESTS := page_table_test

page_table_test.objs := \
    	$(MODULE_BUILD_DIR)/page_table_test.o \
	$(PARENT_BUILD_DIR)/page_table.o
