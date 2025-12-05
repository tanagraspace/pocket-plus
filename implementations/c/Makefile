# POCKET+ C Implementation Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -pedantic -Werror
LDFLAGS = -lm

SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = tests
BUILD_DIR = build

# Source files (exclude CLI which has main())
SRCS = $(filter-out $(SRC_DIR)/cli.c, $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Test files
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(BUILD_DIR)/%)

# Library and CLI
LIB = $(BUILD_DIR)/libpocketplus.a
CLI = $(BUILD_DIR)/pocket_compress

.PHONY: all clean test cli

all: $(LIB) $(CLI)

cli: $(CLI)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJS)
	ar rcs $@ $^

$(CLI): $(SRC_DIR)/cli.c $(LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lpocketplus $(LDFLAGS) -o $@

test: $(TEST_BINS)
	@for test in $(TEST_BINS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done

$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lpocketplus $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR)
