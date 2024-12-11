SHELL := bash


CFLAGS = -Wall -O2
TESTFLAGS = -Wall -g3
CC = gcc

INCLUDE_DIR = include/
BIN_DIR = bin/
SRC_DIR = src/
TESTS_DIR = tests/
CUTEST_DIR = cutest-1.5

TARGET_NAME = proto
TARGET = $(BIN_DIR)/lib/$(TARGET_NAME).ar
TARGET_HEADER = $(BIN_DIR)/lib/$(TARGET_NAME).h

SRCS = $(wildcard $(SRC_DIR)/*.c)
TESTS = $(wildcard $(TESTS_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/obj/%.o,$(SRCS))

default: $(TARGET) $(TARGET_HEADER)

.PHONY:
mkdir:
	mkdir -p $(BIN_DIR)
	mkdir -p $(BIN_DIR)/lib/
	mkdir -p $(BIN_DIR)/obj/
	mkdir -p $(BIN_DIR)/test/

ifndef VERBOSE
.SILENT:
endif

$(BIN_DIR)/obj/%.o: $(SRC_DIR)/%.c
	$(CC) -I$(INCLUDE_DIR) -c $< -o $@ $(CFLAGS)

$(TARGET_HEADER):
	cat $(wildcard $(INCLUDE_DIR)/*.h) > $(TARGET_HEADER)

$(TARGET): $(OBJS)
	ar rvs $(TARGET) $^

$(BIN_DIR)/test/make-tests.sh: $(TESTS)
	cp $(TESTS) $(BIN_DIR)/test/
	cp $(CUTEST_DIR)/make-tests.sh $(BIN_DIR)/test
	chmod +x $(BIN_DIR)/test/make-tests.sh

$(BIN_DIR)/test/AllTests.c: $(BIN_DIR)/test/make-tests.sh
	$(shell cd $(BIN_DIR)/test ; ./make-tests.sh > AllTests.c)
	cat $(wildcard $(TESTS_DIR)/*.c) >> $(BIN_DIR)/test/AllTests.c

$(BIN_DIR)/test/test.o: $(OBJS) $(BIN_DIR)/test/AllTests.c
	./creator.sh tests/test_config > tests/parser_config.h
	$(CC) -o $(BIN_DIR)/test/test.o \
	-I$(CUTEST_DIR) \
	-I$(INCLUDE_DIR) \
	-I$(TESTS_DIR) \
	$(CUTEST_DIR)/CuTest.c \
	$(BIN_DIR)/test/AllTests.c \
	$(OBJS)

test: $(BIN_DIR)/test/test.o
	chmod +x $(BIN_DIR)/test/test.o
	./$(BIN_DIR)/test/test.o
	rm $(BIN_DIR)/test/make-tests.sh &>/dev/null

.PHONY: clean
clean:
	rm $(BIN_DIR)/test/*
	rm $(OBJS)
