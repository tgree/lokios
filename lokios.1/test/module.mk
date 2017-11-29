TESTS += $(TESTDIR)/char_stream_test \
	 $(TESTDIR)/console_test

CHAR_STREAM_TEST_OBJS := \
	$(MODULE_BUILD_DIR)/char_stream_test.o \
	$(PARENT_BUILD_DIR)/char_stream.o
-include $(CHAR_STREAM_TEST_OBJS:.o=.d)

CONSOLE_TEST_OBJS := \
    	$(MODULE_BUILD_DIR)/console_test.o \
	$(PARENT_BUILD_DIR)/console.o \
	$(PARENT_BUILD_DIR)/char_stream.o
-include $(CONSOLE_TEST_OBJS:.o=.d)

$(TESTDIR)/char_stream_test: $(CHAR_STREAM_TEST_OBJS) $(LTMOCK) $(MODULE)/module.mk
	$(BUILD_TEST) $(CHAR_STREAM_TEST_OBJS) $(LTMOCK)

$(TESTDIR)/console_test: $(CONSOLE_TEST_OBJS) $(LTMOCK) $(MODULE)/module.mk
	$(BUILD_TEST) $(CONSOLE_TEST_OBJS) $(LTMOCK)
