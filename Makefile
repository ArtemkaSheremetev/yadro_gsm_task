CC = gcc
CFLAGS = -Wall -Wextra -O3 -Iinc

ifeq ($(OS),Windows_NT)
EXEEXT = .exe
else
EXEEXT =
endif

BUILD_DIR = build
APP_SRC = main.c $(wildcard src/*.c)
OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(APP_SRC))
TARGET = $(BUILD_DIR)/csv_parser$(EXEEXT)
TEST_SRC = $(wildcard tests/*.c)
TEST_TARGETS = $(patsubst tests/%.c,$(BUILD_DIR)/tests/%$(EXEEXT),$(TEST_SRC))

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

$(BUILD_DIR)/tests/%$(EXEEXT): tests/%.c $(wildcard src/*.c) | $(BUILD_DIR) $(BUILD_DIR)/src $(BUILD_DIR)/tests
	$(CC) $(CFLAGS) $< $(wildcard src/*.c) -o $@

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGETS)
