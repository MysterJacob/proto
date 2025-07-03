SHELL := bash

CC ?= gcc
SO_CC ?= $(CC)

COMPILE_FLAGS = -Wall -Os
DEBUG_FLAGS = -Wall -g3

INCLUDE_DIR = include/
BUILD_DIR = bin/
LIB_DIR = $(BUILD_DIR)lib/
OBJ_DIR = $(BUILD_DIR)obj/
SRC_DIR = src/

SOURCES = $(wildcard $(SRC_DIR)*.c)

TESTS_DIR = tests/
CUTEST_DIR = cutest-1.5/

SO_FILE = $(LIB_DIR)proto.so
AR_FILE = $(LIB_DIR)proto.ar
TEST_FILE = $(BUILD_DIR)test/test.o
TARGET_HEADER = $(LIB_DIR)proto.h
PYTHON_FILE = $(LIB_DIR)proto.py

CONFIG := config

default: headers $(SO_FILE)
all: headers $(SO_FILE) $(AR_FILE) $(PYTHON_FILE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)lib/
	mkdir -p $(BUILD_DIR)obj/
	mkdir -p $(BUILD_DIR)test/

.PHONY:
headers: $(BUILD_DIR) 
	./creator.sh $(CONFIG) $(INCLUDE_DIR)parserTables.h $(INCLUDE_DIR)packets.h $(INCLUDE_DIR)config.h

	cp $(INCLUDE_DIR)proto.h $(TARGET_HEADER)

	sed -i -e '/#include \"config.h\"/{r include/config.h' -e 'd}' $(TARGET_HEADER)
	sed -i -e '/#include \"datatypes.h\"/{r include/datatypes.h' -e 'd}' $(TARGET_HEADER)
	sed -i -e '/#include \"packets.h\"/{r include/packets.h' -e 'd}' $(TARGET_HEADER)

$(SO_FILE): $(BUILD_DIR) headers $(SOURCES)
	$(SO_CC) $(COMPILE_FLAGS) \
	-shared \
	-fPIC \
	-o $(SO_FILE) \
	-I$(INCLUDE_DIR) $(SOURCES)

$(AR_FILE): $(BUILD_DIR) headers $(SOURCES)
	$(CC) $(COMPILE_FLAGS) \
	-fPIC \
	-I$(INCLUDE_DIR) \
	-c $(SOURCES) \
	-o $(OBJ_DIR)proto.o

	ar rvs $(AR_FILE) $(OBJ_DIR)proto.o

$(PYTHON_FILE): $(BUILD_DIR)
	./pythoncreator.sh config proto.py $(PYTHON_FILE)

.PHONY:
clean:
	rm -rf $(BUILD_DIR)

TESTS = $(wildcard $(TESTS_DIR)*.c)
$(TEST_FILE): headers $(TESTS_DIR)config $(SOURCES) $(AR_FILE)
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

	chmod +x $(TEST_FILE) 

.PHONY:
testcfg:
	$(eval CONFIG := $(TESTS_DIR)config)

.PHONY:
test: testcfg $(TEST_FILE)
	./$(TEST_FILE)

debug: testcfg $(TEST_FILE)
	cp $(TEST_FILE) debug
