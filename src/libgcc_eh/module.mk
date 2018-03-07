GCC_SRCS_DIR := /usr/src/gcc-7/gcc-7.2.0
GCC_INCLUDE_DIR := $(GCC_SRCS_DIR)/include
GCC_LIBGCC_SRCS_DIR := $(GCC_SRCS_DIR)/libgcc
LOCAL_SRCS_DIR := $(abspath $(MODULE_SRC_DIR))

LIBSUPCXX_SRC := $(GCC_LIBGCC_SRCS_DIR)/unwind-dw2.c \
                 $(GCC_LIBGCC_SRCS_DIR)/unwind-dw2-fde.c \
                 $(GCC_LIBGCC_SRCS_DIR)/unwind-sjlj.c \
                 $(GCC_LIBGCC_SRCS_DIR)/unwind-c.c
		 
LIBGCC_EH_OBJ := $(LIBSUPCXX_SRC:$(GCC_LIBGCC_SRCS_DIR)/%.c=$(MODULE_BUILD_DIR)/%.o)

LIBGCC_EH_CFLAGS := -O2 -march=core2 -m64 -Wall -Werror \
                -Wno-multichar \
                -ggdb -mcmodel=kernel --sysroot=/home/greent7/sysroot \
                -mno-sse \
                -isystem =/usr/include/c++/7 \
                -isystem =/usr/include/x86_64-linux-gnu/c++/7 \
                -isystem =/usr/include/c++/7/backward \
                -isystem =/usr/lib/gcc/x86_64-linux-gnu/7/include \
                -isystem =/usr/lib/gcc/x86_64-linux-gnu/7/include-fixed

$(MODULE_BUILD_DIR)/%.o: $(GCC_LIBGCC_SRCS_DIR)/%.c
	@echo Compiling $^...
	@mkdir -p $(dir $@)
	$(CC) $(LIBGCC_EH_CFLAGS) \
	      -I$(GCC_LIBGCC_SRCS_DIR) \
	      -I$(GCC_INCLUDE_DIR) \
	      -I$(LOCAL_SRCS_DIR) \
	      -c $^ -o $@

$(LIB_DIR)/libgcc_eh.a: $(LIBGCC_EH_OBJ) $(MODULE_MK)
	@echo Archiving $@...
	@mkdir -p $(LIB_DIR)
	@$(AR) $(ARFLAGS) $@ $(LIBGCC_EH_OBJ)
