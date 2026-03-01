SHELL := bash

CC ?= gcc
CONFIG ?= config.cfg
CONFIG_DIR = $(dir, $(CONFIG))

COMPILE_FLAGS = -Wall -Os
DEBUG_FLAGS = -Wall -g3 -pg

INCLUDE_DIR = include/
BUILD_DIR = bin/
LIB_DIR = $(BUILD_DIR)lib/
OBJ_DIR = $(BUILD_DIR)obj/
TEST_DIR = $(BUILD_DIR)tests/
SRC_DIR = src/

CUTEST_DIR = cutest-1.5/
TEST_SOURCES_DIR = tests/
TEST_CONFIG_DIR = $(TEST_SOURCES_DIR)configs/
TEST_SOURCES = $(wildcard $(TEST_SOURCES_DIR)*.c)
TEST_CONFIGS = $(wildcard $(TEST_SOURCES_DIR)configs/*.cfg)
TEST_FILES = $(patsubst $(TEST_SOURCES_DIR)configs/%.cfg, $(TEST_DIR)%.o, $(TEST_CONFIGS))

SOURCES = $(wildcard $(SRC_DIR)*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)%.c, $(OBJ_DIR)%.o, $(SOURCES))

SO_FILE = $(LIB_DIR)proto.so
AR_FILE = $(LIB_DIR)proto.ar

TARGET_HEADER = $(LIB_DIR)proto.h
PYTHON_FILE = $(LIB_DIR)proto.py

.FORCE:
default: $(CONFIG) $(SO_FILE)
all: $(SO_FILE) $(PYTHON_FILE)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(TEST_DIR)

.FORCE:
$(CONFIG_DIR)%.cfg $(TEST_CONFIG_DIR)%.cfg: $(BUILD_DIR)
	./creator.sh $@ $(INCLUDE_DIR)parserTables.h $(INCLUDE_DIR)packets.h $(INCLUDE_DIR)config.h

	@gcc -I$(INCLUDE_DIR) -nostdlib -DHEADER_COMPILATION -E include/proto.h -o $(TARGET_HEADER)
	@awk -i inplace -F" " 'BEGINFILE{print "#define CONFIG_NAME $@\n#include <stdint.h>\n#include <stddef.h>"} /^[^#]/{print $0}' $(TARGET_HEADER)

$(SO_FILE): $(BUILD_DIR) $(CONFIG) $(OBJ_FILES)
	$(CC) $(COMPILE_FLAGS) -shared -o $(LIB_DIR)libproto.so $(OBJ_FILES)
	ar rvs $(AR_FILE) $(OBJ_FILES)

$(OBJ_DIR)%.o: $(SRC_DIR)%.c $(CONFIG) $(INCLUDE_DIR)config.h
	$(CC) $(COMPILE_FLAGS) -I$(INCLUDE_DIR) -c -fPIC $< -o $@

$(TEST_DIR)%.o: $(TEST_CONFIG_DIR)%.cfg $(TEST_SOURCES)
	@mkdir -p $(BUILD_DIR)tmp/

	@cp $(TEST_SOURCES) \
	$(CUTEST_DIR)CuTest.h \
	$(CUTEST_DIR)make-tests.sh \
	$(BUILD_DIR)tmp/

	@chmod +x $(BUILD_DIR)tmp/make-tests.sh

	@(cd $(BUILD_DIR)tmp ; ./make-tests.sh > AllTests.c)

	@$(CC) $(DEBUG_FLAGS) \
	$(SOURCES) \
	-o $@ \
	-I$(CUTEST_DIR) \
	-I$(INCLUDE_DIR) \
	-I$(TEST_SOURCES_DIR) \
	$(TEST_SOURCES) \
	$(CUTEST_DIR)CuTest.c \
	$(BUILD_DIR)tmp/AllTests.c

	@rm -rf $(BUILD_DIR)tmp/

.PHONY:
test: $(BUILD_DIR) $(TEST_FILES)
	@for testFile in $(TEST_FILES); do \
		echo Testing $$testFile ;\
		./$$testFile; \
		echo Result: $$?; \
	done

.PHONY:
clean:
	rm -rf $(BUILD_DIR)
	rm -f debug
	rm -f gmon.out
	rm include/config.h
	rm include/packets.h
	rm include/parserTables.h

debug: $(TEST_DIR)nopreamble.o
	cp $(TEST_DIR)nopreamble.o debug

profile:
	gprof ./bin/test/test.o gmon.out
