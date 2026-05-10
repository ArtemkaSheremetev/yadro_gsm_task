CC = gcc
CFLAGS = -Wall -Wextra -O3 -Iinc

BUILD_DIR = build
APP_SRC = main.c $(wildcard src/*.c)
OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRC))
TARGET = $(BUILD_DIR)/csv_parser
TEST_SRC = $(wildcard tests/*.c)
TEST_TARGETS = $(patsubst tests/%.c,$(BUILD_DIR)/tests/%,$(TEST_SRC))

all: $(TARGET)

test: $(TEST_TARGETS)
	for test_bin in $(TEST_TARGETS); do ./$$test_bin; done

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/src:
	mkdir -p $(BUILD_DIR)/src

$(BUILD_DIR)/tests:
	mkdir -p $(BUILD_DIR)/tests

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR) $(BUILD_DIR)/src
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%: tests/%.c $(wildcard src/*.c) | $(BUILD_DIR) $(BUILD_DIR)/src $(BUILD_DIR)/tests
	$(CC) $(CFLAGS) $< $(wildcard src/*.c) -o $@

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGETS)
