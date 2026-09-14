# Build & directory configuration
MODE ?= release
PROGRAM_NAME = clox

SRC_DIR = src

DIST_DIR = dist

TARGET_DIR = $(DIST_DIR)/$(MODE)
VENDOR_DIR = vendor

TARGET = $(TARGET_DIR)/$(PROGRAM_NAME)
OBJ_DIR = $(TARGET_DIR)/obj

VENDOR_HEADER_DIR = vendor


_default: $(TARGET)


# Build flags
CFLAGS_COMMON = -std=c11 -Wall -Wextra -pedantic

CFLAGS_release = $(CFLAGS_COMMON) -O3 -flto
CFLAGS_debug = $(CFLAGS_COMMON) -O0 -g3
CFLAGS_test = $(CFLAGS_COMMON) -O0 -g3 --coverage

CFLAGS = $(CFLAGS_$(MODE))

ifeq ($(CFLAGS),)
	$(error "ERROR: unknown build mode $(MODE)")
endif


# Data structures library, exposes LIBFUN target.
LIBFUN_DIR := $(VENDOR_DIR)/libfun

LIBFUN_MODE := release

$(LIBFUN_DIR)/libfun.mk: | $(LIBFUN_DIR)/
	cd $(LIBFUN_DIR)/ && \
		git init -q && \
		git remote add origin https://github.com/metwse/libfun.git && \
		git fetch --depth 1 origin 97ea792a15f341686c7058ba2b8d7ed4a7e0b62f && \
		git checkout -q FETCH_HEAD

include $(LIBFUN_DIR)/libfun.mk


SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

$(TARGET): $(OBJS) $(LIBFUN) | $(TARGET_DIR)/
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(DIST_DIR)%/:
	mkdir -p $@
$(VENDOR_DIR)%/:
	mkdir -p $@


clean:
	$(RM) -r $(DIST_DIR)

clean-vendor:
	$(RM) -r $(VENDOR_DIR)

.PHONY: _default clean clean-vendor

-include $(OBJS:.o=.d)
