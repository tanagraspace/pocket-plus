# POCKET+ C Implementation Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude -pedantic -Werror
LDFLAGS = -lm

# Coverage flags
COV_CFLAGS = $(CFLAGS) --coverage -fprofile-arcs -ftest-coverage
COV_LDFLAGS = $(LDFLAGS) --coverage

SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = tests
BUILD_DIR = build
COV_DIR = $(BUILD_DIR)/coverage

# Source files (exclude CLI which has main())
SRCS = $(filter-out $(SRC_DIR)/cli.c, $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
COV_OBJS = $(SRCS:$(SRC_DIR)/%.c=$(COV_DIR)/%.o)

# Test files
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(BUILD_DIR)/%)
COV_TEST_BINS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(COV_DIR)/%)

# Library and CLI
LIB = $(BUILD_DIR)/libpocketplus.a
CLI = $(BUILD_DIR)/pocketplus
COV_LIB = $(COV_DIR)/libpocketplus.a

.PHONY: all clean test test-cli cli coverage coverage-html docs

all: $(LIB) $(CLI)

cli: $(CLI)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(COV_DIR):
	mkdir -p $(COV_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(COV_DIR)/%.o: $(SRC_DIR)/%.c | $(COV_DIR)
	$(CC) $(COV_CFLAGS) -c $< -o $@

$(LIB): $(OBJS)
	ar rcs $@ $^

$(COV_LIB): $(COV_OBJS)
	ar rcs $@ $^

$(CLI): $(SRC_DIR)/cli.c $(LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lpocketplus $(LDFLAGS) -o $@

test: $(TEST_BINS)
	@for test in $(TEST_BINS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done

test-cli: $(CLI)
	@$(TEST_DIR)/test_cli.sh

$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lpocketplus $(LDFLAGS) -o $@

$(COV_DIR)/%: $(TEST_DIR)/%.c $(COV_LIB) | $(COV_DIR)
	$(CC) $(COV_CFLAGS) $< -L$(COV_DIR) -lpocketplus $(COV_LDFLAGS) -o $@

coverage: $(COV_TEST_BINS)
	@echo "Running tests with coverage..."
	@for test in $(COV_TEST_BINS); do \
		$$test || exit 1; \
	done
	@echo ""
	@echo "Coverage Summary:"
	@echo "================="
	@gcov -o $(COV_DIR) $(SRCS) 2>/dev/null | grep -A1 "^File.*src/" | grep -v "^--$$"
	@$(MAKE) --no-print-directory coverage-badge

coverage-badge:
	@mkdir -p assets
	@# Generate coverage.info first for accurate stats
	@lcov --capture --directory $(COV_DIR) --output-file $(COV_DIR)/coverage.info --include "*/src/*" 2>/dev/null || true
	@# Extract line and function coverage from lcov summary
	@lcov --summary $(COV_DIR)/coverage.info 2>&1 | grep "lines" | sed 's/.*: //' | sed 's/%.*//' > $(COV_DIR)/.lines_pct
	@lcov --summary $(COV_DIR)/coverage.info 2>&1 | grep "functions" | sed 's/.*: //' | sed 's/%.*//' > $(COV_DIR)/.funcs_pct
	@# Generate lines badge
	@PCT=$$(cat $(COV_DIR)/.lines_pct); \
	if [ $$(echo "$$PCT >= 90" | bc) -eq 1 ]; then COLOR="brightgreen"; \
	elif [ $$(echo "$$PCT >= 80" | bc) -eq 1 ]; then COLOR="green"; \
	elif [ $$(echo "$$PCT >= 70" | bc) -eq 1 ]; then COLOR="yellowgreen"; \
	elif [ $$(echo "$$PCT >= 60" | bc) -eq 1 ]; then COLOR="yellow"; \
	else COLOR="red"; fi; \
	echo '<svg xmlns="http://www.w3.org/2000/svg" width="90" height="20">' > assets/coverage-lines.svg; \
	echo '<linearGradient id="b" x2="0" y2="100%"><stop offset="0" stop-color="#bbb" stop-opacity=".1"/><stop offset="1" stop-opacity=".1"/></linearGradient>' >> assets/coverage-lines.svg; \
	echo '<mask id="a"><rect width="90" height="20" rx="3" fill="#fff"/></mask>' >> assets/coverage-lines.svg; \
	echo '<g mask="url(#a)"><path fill="#555" d="M0 0h45v20H0z"/><path fill="'$$COLOR'" d="M45 0h45v20H45z"/><path fill="url(#b)" d="M0 0h90v20H0z"/></g>' >> assets/coverage-lines.svg; \
	echo '<g fill="#fff" text-anchor="middle" font-family="DejaVu Sans,Verdana,Geneva,sans-serif" font-size="11">' >> assets/coverage-lines.svg; \
	echo '<text x="22.5" y="15" fill="#010101" fill-opacity=".3">lines</text>' >> assets/coverage-lines.svg; \
	echo '<text x="22.5" y="14">lines</text>' >> assets/coverage-lines.svg; \
	echo '<text x="67" y="15" fill="#010101" fill-opacity=".3">'$$PCT'%</text>' >> assets/coverage-lines.svg; \
	echo '<text x="67" y="14">'$$PCT'%</text></g></svg>' >> assets/coverage-lines.svg; \
	echo "Badge: assets/coverage-lines.svg ($$PCT%)"
	@# Generate functions badge
	@PCT=$$(cat $(COV_DIR)/.funcs_pct); \
	if [ $$(echo "$$PCT >= 90" | bc) -eq 1 ]; then COLOR="brightgreen"; \
	elif [ $$(echo "$$PCT >= 80" | bc) -eq 1 ]; then COLOR="green"; \
	elif [ $$(echo "$$PCT >= 70" | bc) -eq 1 ]; then COLOR="yellowgreen"; \
	elif [ $$(echo "$$PCT >= 60" | bc) -eq 1 ]; then COLOR="yellow"; \
	else COLOR="red"; fi; \
	echo '<svg xmlns="http://www.w3.org/2000/svg" width="106" height="20">' > assets/coverage-functions.svg; \
	echo '<linearGradient id="b" x2="0" y2="100%"><stop offset="0" stop-color="#bbb" stop-opacity=".1"/><stop offset="1" stop-opacity=".1"/></linearGradient>' >> assets/coverage-functions.svg; \
	echo '<mask id="a"><rect width="106" height="20" rx="3" fill="#fff"/></mask>' >> assets/coverage-functions.svg; \
	echo '<g mask="url(#a)"><path fill="#555" d="M0 0h61v20H0z"/><path fill="'$$COLOR'" d="M61 0h45v20H61z"/><path fill="url(#b)" d="M0 0h106v20H0z"/></g>' >> assets/coverage-functions.svg; \
	echo '<g fill="#fff" text-anchor="middle" font-family="DejaVu Sans,Verdana,Geneva,sans-serif" font-size="11">' >> assets/coverage-functions.svg; \
	echo '<text x="30.5" y="15" fill="#010101" fill-opacity=".3">functions</text>' >> assets/coverage-functions.svg; \
	echo '<text x="30.5" y="14">functions</text>' >> assets/coverage-functions.svg; \
	echo '<text x="82.5" y="15" fill="#010101" fill-opacity=".3">'$$PCT'%</text>' >> assets/coverage-functions.svg; \
	echo '<text x="82.5" y="14">'$$PCT'%</text></g></svg>' >> assets/coverage-functions.svg; \
	echo "Badge: assets/coverage-functions.svg ($$PCT%)"

coverage-html:
	@command -v lcov >/dev/null 2>&1 || { echo "Error: lcov not installed. Install with: brew install lcov"; exit 1; }
	@mkdir -p $(COV_DIR)/html
	@if [ ! -f $(COV_DIR)/coverage.info ]; then \
		lcov --capture --directory $(COV_DIR) --output-file $(COV_DIR)/coverage.info --include "*/src/*"; \
	fi
	genhtml $(COV_DIR)/coverage.info --output-directory $(COV_DIR)/html
	@echo "HTML report: $(COV_DIR)/html/index.html"

docs:
	@command -v doxygen >/dev/null 2>&1 || { echo "Error: doxygen not installed. Install with: brew install doxygen"; exit 1; }
	@mkdir -p $(BUILD_DIR)/docs
	@# Download Doxygen Awesome CSS if not present
	@if [ ! -f doxygen-awesome-css/doxygen-awesome.css ]; then \
		echo "Downloading Doxygen Awesome CSS theme..."; \
		mkdir -p doxygen-awesome-css; \
		curl -sL https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/v2.3.4/doxygen-awesome.css -o doxygen-awesome-css/doxygen-awesome.css; \
		curl -sL https://raw.githubusercontent.com/jothepro/doxygen-awesome-css/v2.3.4/doxygen-awesome-sidebar-only.css -o doxygen-awesome-css/doxygen-awesome-sidebar-only.css; \
	fi
	doxygen Doxyfile
	@echo "Documentation: $(BUILD_DIR)/docs/html/index.html"

clean:
	rm -rf $(BUILD_DIR)/* $(BUILD_DIR)/.[!.]* 2>/dev/null || true
