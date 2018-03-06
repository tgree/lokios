GCC_SRCS_DIR := /usr/src/gcc-7/gcc-7.2.0
GCC_LIBSUPCXX_SRCS_DIR := $(GCC_SRCS_DIR)/libstdc++-v3/libsupc++

LIBSUPCXX_SRC := $(wildcard $(GCC_LIBSUPCXX_SRCS_DIR)/*.cc)
LIBSUPCXX_OBJ := $(LIBSUPCXX_SRC:$(GCC_LIBSUPCXX_SRCS_DIR)/%.cc=$(MODULE_BUILD_DIR)/%.o) $(MODULE_BUILD_DIR)/cp-demangle.o

$(MODULE_BUILD_DIR)/%.o: $(GCC_LIBSUPCXX_SRCS_DIR)/%.cc
	@echo Compiling $^...
	@mkdir -p $(dir $@)
	@$(CXX) $(BASE_KERN_CXXFLAGS) -Wno-unused-result \
	        -I$(GCC_SRCS_DIR)/libgcc -I$(GCC_SRCS_DIR)/include \
		-c $^ -o $@

$(MODULE_BUILD_DIR)/cp-demangle.o: $(GCC_SRCS_DIR)/libiberty/cp-demangle.c
	@echo Compiling $^...
	@mkdir -p $(dir $@)
	@$(CC) $(KERN_CCFLAGS) \
	        -I$(GCC_SRCS_DIR)/libgcc -I$(GCC_SRCS_DIR)/include \
	       -DHAVE_STRING_H -DHAVE_DECL_BASENAME -DHAVE_STDLIB_H -DIN_GLIBCPP_V3 \
	       -c $^ -o $@

$(LIB_DIR)/libsupc++.a: $(LIBSUPCXX_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(LIBSUPCXX_OBJ)
