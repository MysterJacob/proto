SHELL := bash

CC ?= gcc
CONFIG ?= config

COMPILE_FLAGS = -Wall -Os
DEBUG_FLAGS = -Wall -g3 -pg

INCLUDE_DIR = include/
BUILD_DIR = bin/
LIB_DIR = $(BUILD_DIR)lib/
OBJ_DIR = $(BUILD_DIR)obj/
SRC_DIR = src/

SOURCES = $(wildcard $(SRC_DIR)*.c)

TESTS_DIR = tests/
CUTEST_DIR = cutest-1.5/

OBJ_FILES = $(patsubst $(SRC_DIR)%.c, $(OBJ_DIR)%.o, $(SOURCES))
SO_FILE = $(LIB_DIR)proto.so
AR_FILE = $(LIB_DIR)proto.ar
TEST_FILE = $(BUILD_DIR)test/test.o
TARGET_HEADER = $(LIB_DIR)proto.h
PYTHON_FILE = $(LIB_DIR)proto.py


default: headers $(SO_FILE)
all: headers $(SO_FILE) $(PYTHON_FILE)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)lib/
	@mkdir -p $(BUILD_DIR)obj/
	@mkdir -p $(BUILD_DIR)test/

.PHONY:
headers: $(CONFIG) $(BUILD_DIR)
	@./creator.sh $(CONFIG) $(INCLUDE_DIR)parserTables.h $(INCLUDE_DIR)packets.h $(INCLUDE_DIR)config.h

	@gcc -I$(INCLUDE_DIR) -nostdlib -DHEADER_COMPILATION -E include/proto.h -o $(TARGET_HEADER)
	@awk -i inplace -F" " 'BEGINFILE{print "#include <stdint.h>\n#include <stddef.h>"} /^[^#]/{print $0}' $(TARGET_HEADER)

$(SO_FILE): $(BUILD_DIR) headers $(OBJ_FILES)
	$(CC) $(COMPILE_FLAGS) -shared -o $(LIB_DIR)libproto.so $(OBJ_FILES)
	ar rvs $(AR_FILE) $(OBJ_FILES)

$(OBJ_DIR)%.o: $(SRC_DIR)%.c $(CONFIG)
	$(CC) $(COMPILE_FLAGS) -I$(INCLUDE_DIR) -c -fPIC $< -o $@

$(PYTHON_FILE): $(BUILD_DIR) $(CONFIG)
	@./pythoncreator.sh config proto.py $(PYTHON_FILE)

.PHONY:
clean:
	rm -rf $(BUILD_DIR)
	rm -f debug
	rm -f gmon.out

TESTS = $(wildcard $(TESTS_DIR)*.c)
$(TEST_FILE): headers $(TESTS_DIR)config $(SOURCES) 
	cp \
	$(TESTS) \
	$(CUTEST_DIR)CuTest.h \
	$(CUTEST_DIR)make-tests.sh \
	$(BUILD_DIR)test

	chmod +x $(BUILD_DIR)test/make-tests.sh
	
	(cd $(BUILD_DIR)test ; rm AllTests.c ; ./make-tests.sh > AllTests.c)

	$(CC) $(DEBUG_FLAGS) \
	$(SOURCES) \
	-o $(TEST_FILE) \
	-I$(CUTEST_DIR) \
	-I$(INCLUDE_DIR) \
	-I$(TESTS_DIR) \
	$(TESTS) \
	$(CUTEST_DIR)CuTest.c \
	$(BUILD_DIR)test/AllTests.c \

.PHONY:
testcfg:
	$(eval CONFIG := $(TESTS_DIR)config)

.PHONY:
test: testcfg $(TEST_FILE)
		valgrind --leak-check=full --show-leak-kinds=all ./bin/test/test.o || \
		(./$(TEST_FILE); echo WARNING! Valgrind has not been found, ran only plain test without memory leaks check.)

debug: testcfg $(TEST_FILE)
	cp $(TEST_FILE) debug
