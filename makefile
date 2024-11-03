CFLAGS = -Wall -O2
CC = gcc

INCLUDE_DIR = include/
BIN_DIR = bin/
SRC_DIR = src/

TARGET_NAME = proto
TARGET = $(BIN_DIR)/lib/$(TARGET_NAME).ar
TARGET_HEADER = $(BIN_DIR)/lib/$(TARGET_NAME).h

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/obj/%.o,$(SRCS))

default: $(TARGET) $(TARGET_HEADER)

mkdir:
	mkdir -p $(BIN_DIR)
	mkdir -p $(BIN_DIR)/lib/
	mkdir -p $(BIN_DIR)/obj/
	mkdir -p $(BIN_DIR)/test/

$(BIN_DIR)/obj/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(TARGET_HEADER):
	cat $(wildcard $(INCLUDE_DIR)/*.h) > $(TARGET_HEADER)

$(TARGET): $(OBJS)
	ar rvs $(TARGET) $^

test:


.PHONY: clean
clean:
	rm $(OBJS)
