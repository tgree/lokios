TESTS += $(TESTS_DIR)/char_stream_test \
	 $(TESTS_DIR)/console_test \
	 $(TESTS_DIR)/klist_test

CHAR_STREAM_TEST_OBJS := \
	$(MODULE_BUILD_DIR)/char_stream_test.o \
	$(PARENT_BUILD_DIR)/char_stream.o
-include $(CHAR_STREAM_TEST_OBJS:.o=.d)

CONSOLE_TEST_OBJS := \
    	$(MODULE_BUILD_DIR)/console_test.o \
	$(PARENT_BUILD_DIR)/console.o \
	$(PARENT_BUILD_DIR)/char_stream.o
-include $(CONSOLE_TEST_OBJS:.o=.d)

KLIST_TEST_OBJS := \
    	$(MODULE_BUILD_DIR)/klist_test.o
-include $(KLIST_TEST_OBJS:.o=.d)

$(TESTS_DIR)/char_stream_test: $(CHAR_STREAM_TEST_OBJS) $(LTMOCK) $(MODULE_MK)
	$(BUILD_TEST) $(CHAR_STREAM_TEST_OBJS) $(LTMOCK)

$(TESTS_DIR)/console_test: $(CONSOLE_TEST_OBJS) $(LTMOCK) $(MODULE_MK)
	$(BUILD_TEST) $(CONSOLE_TEST_OBJS) $(LTMOCK)

$(TESTS_DIR)/klist_test: $(KLIST_TEST_OBJS) $(LTMOCK) $(MODULE_MK)
	$(BUILD_TEST) $(KLIST_TEST_OBJS) $(LTMOCK)
