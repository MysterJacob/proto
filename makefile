SHELL := bash

PLATFORM ?= 
CC ?= gcc
AR ?= ar
CONFIG ?= config.cfg
CONFIG_DIR = $(dir, $(CONFIG))

EXTRA_FLAGS ?= 
COMPILE_FLAGS = -Wall -Wextra -Os -pedantic -fPIC $(EXTRA_FLAGS)
DEBUG_FLAGS = -Wall -Wextra -g3 -pg -pedantic --std=c2x

SRC_DIR = src/
INCLUDE_DIR = include/
BUILD_DIR = bin/
PLATFORM_DIR = $(BUILD_DIR)$(PLATFORM)/
LIB_DIR = $(PLATFORM_DIR)lib/
OBJ_DIR = $(PLATFORM_DIR)obj/
TEST_DIR = $(PLATFORM_DIR)tests/
PYTHON_SRC_DIR = python/

CUTEST_DIR = cutest-1.5/
TEST_SOURCES_DIR = tests/
TEST_CONFIG_DIR = $(TEST_SOURCES_DIR)configs/
TEST_SOURCES = $(wildcard $(TEST_SOURCES_DIR)*.c)
TEST_CONFIGS = $(wildcard $(TEST_SOURCES_DIR)configs/*.cfg)
TEST_FILES = $(patsubst $(TEST_SOURCES_DIR)configs/%.cfg, $(TEST_DIR)%.o, $(TEST_CONFIGS))

SOURCES = $(wildcard $(SRC_DIR)*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)%.c, $(OBJ_DIR)%.o, $(SOURCES))

SO_FILE = $(LIB_DIR)libproto.so
AR_FILE = $(LIB_DIR)libproto.a

TARGET_HEADER = $(LIB_DIR)proto.h
PYTHON_DIR = $(BUILD_DIR)python/
PYTHON_FILE = $(PYTHON_DIR)proto.py

.FORCE:
default: $(CONFIG) $(SO_FILE) $(AR_FILE)

.PHONY:
python-bridge:
	make PLATFORM=python $(PYTHON_FILE)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(PYTHON_DIR): $(BUILD_DIR)
	@mkdir -p $(PYTHON_DIR)

$(PLATFORM_DIR): $(BUILD_DIR)
	@mkdir -p $(PLATFORM_DIR)
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(TEST_DIR)

.FORCE:
$(CONFIG_DIR)%.cfg $(TEST_CONFIG_DIR)%.cfg: $(PLATFORM_DIR)
	@./creator.sh $@ $(INCLUDE_DIR)parserTables.h $(INCLUDE_DIR)packets.h $(INCLUDE_DIR)config.h

	@$(CC) -I$(INCLUDE_DIR) -nostdlib -DHEADER_COMPILATION -E include/proto.h -o $(TARGET_HEADER)
	@awk -i inplace -F" " 'BEGINFILE{print "#define CONFIG_NAME $@\n#include <stdint.h>\n#include <stddef.h>"} /^[^#]/{print $0}' $(TARGET_HEADER)

$(SO_FILE): $(PLATFORM_DIR) $(CONFIG) $(OBJ_FILES)
	@$(CC) $(COMPILE_FLAGS) -shared -o $(SO_FILE) $(OBJ_FILES) || true

$(AR_FILE): $(PLATFORM_DIR) $(CONFIG) $(OBJ_FILES)
	$(AR) rvs $(AR_FILE) $(OBJ_FILES)

$(OBJ_DIR)%.o: $(SRC_DIR)%.c $(CONFIG) $(INCLUDE_DIR)config.h
	$(CC) $(COMPILE_FLAGS)  -I$(INCLUDE_DIR) -c $< -o $@

$(PYTHON_FILE): $(PYTHON_DIR)
	./$(PYTHON_SRC_DIR)/pythoncreator.sh $(CONFIG) $(PYTHON_SRC_DIR)proto_template.py $(PYTHON_FILE)

$(TEST_DIR)%.o: $(TEST_CONFIG_DIR)%.cfg $(TEST_SOURCES)
	@mkdir -p $(PLATFORM_DIR)tmp/

	@cp $(TEST_SOURCES) \
	$(CUTEST_DIR)CuTest.h \
	$(CUTEST_DIR)make-tests.sh \
	$(PLATFORM_DIR)tmp/

	@chmod +x $(PLATFORM_DIR)tmp/make-tests.sh

	@(cd $(PLATFORM_DIR)tmp ; ./make-tests.sh > AllTests.c)

	@$(CC) $(DEBUG_FLAGS) \
	$(SOURCES) \
	-o $@ \
	-I$(CUTEST_DIR) \
	-I$(INCLUDE_DIR) \
	-I$(TEST_SOURCES_DIR) \
	$(TEST_SOURCES) \
	$(CUTEST_DIR)CuTest.c \
	$(PLATFORM_DIR)tmp/AllTests.c

	@rm -rf $(PLATFORM_DIR)tmp/


.PHONY:
test: $(PLATFORM_DIR) $(TEST_FILES)
	@for testFile in $(TEST_FILES); do \
		echo Testing $$testFile ;\
		./$$testFile; \
		echo Result: $$?; \
	done

.PHONY:
clean:
	rm -rf $(PLATFORM_DIR)
	rm -f debug
	rm -f gmon.out
	rm -f include/config.h include/packets.h include/parserTables.h
	rm -f $(PYTHON_FILE)

.PHONY:
debug: $(TEST_DIR)debug.o
	cp $(TEST_DIR)debug.o debug

.PHONY:
profile:
	gprof ./bin/test/test.o gmon.out
