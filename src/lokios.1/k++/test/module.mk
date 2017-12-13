TESTS := char_stream_test \
	 klist_test       \
	 sort_test        \
	 region_set_test  \
	 vector_test      \
	 kmath_test       \

char_stream_test.objs := \
	$(MODULE_BUILD_DIR)/char_stream_test.o \
	$(PARENT_BUILD_DIR)/char_stream.o

klist_test.objs := \
    	$(MODULE_BUILD_DIR)/klist_test.o

sort_test.objs := \
    	$(MODULE_BUILD_DIR)/sort_test.o

region_set_test.objs := \
    	$(MODULE_BUILD_DIR)/region_set_test.o

vector_test.objs := \
    	$(MODULE_BUILD_DIR)/vector_test.o

kmath_test.objs := \
    	$(MODULE_BUILD_DIR)/kmath_test.o
