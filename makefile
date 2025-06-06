SHELL := bash

CC ?= gcc
LIB_CC ?= $(CC)

CFLAGS = -Wall -Os
DEBUGFLAGS = -Wall -g3

INCLUDE_DIR = include/
BIN_DIR = bin/
SRC_DIR = src/
TESTS_DIR = tests/
CUTEST_DIR = cutest-1.5/

CONFIG := config
TARGET_NAME = proto
TARGET = $(BIN_DIR)/lib/$(TARGET_NAME).ar
TARGET_HEADER = $(BIN_DIR)/lib/$(TARGET_NAME).h
PACKETS_HEADER = $(BIN_DIR)/lib/$(TARGET_NAME).h

SRCS = $(wildcard $(SRC_DIR)/*.c)
TESTS = $(wildcard $(TESTS_DIR)*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)obj/%.o,$(SRCS))

default: $(TARGET)

.PHONY:
mkdir:
	mkdir -p $(BIN_DIR)
	mkdir -p $(BIN_DIR)/lib/
	mkdir -p $(BIN_DIR)/obj/
	mkdir -p $(BIN_DIR)/test/

.PHONY:
clean:
	rm -rf $(BIN_DIR)

$(INCLUDE_DIR)parserTables.h: FORCE
	./creator.sh $(CONFIG) $(INCLUDE_DIR)parserTables.h $(INCLUDE_DIR)packets.h $(INCLUDE_DIR)config.h

FORCE:
$(TARGET): mkdir $(TARGET_HEADER) $(INCLUDE_DIR)parserTables.h 
	$(LIB_CC) $(CFLAGS) \
	-shared \
	-fPIC \
	-o $(BIN_DIR)obj/proto.so \
	-I$(INCLUDE_DIR) $(SRCS)

	$(CC) $(CFLAGS) \
	-fPIC \
	-I$(INCLUDE_DIR) \
	-c $(SRCS) \
	-o $(BIN_DIR)obj/proto.o

	ar rvs $(TARGET) $(BIN_DIR)obj/proto.o
	cp $(BIN_DIR)obj/proto.so bin/lib/proto.so

$(TARGET_HEADER):
	cp $(INCLUDE_DIR)$(TARGET_NAME).h $(TARGET_HEADER)
# 	cp $(INCLUDE_DIR)packets.h $(PACKETS_HEADER)
	sed -i -e '/#include \"config.h\"/{r include/config.h' -e 'd}' $(TARGET_HEADER)
	sed -i -e '/#include \"datatypes.h\"/{r include/datatypes.h' -e 'd}' $(TARGET_HEADER)
	sed -i -e '/#include \"packets.h\"/{r include/packets.h' -e 'd}' $(TARGET_HEADER)

.PHONY:
mktest: changecfg $(TARGET)
	cp \
	$(TESTS) \
	$(CUTEST_DIR)CuTest.h \
	$(CUTEST_DIR)make-tests.sh \
	$(BIN_DIR)test

	chmod +x $(BIN_DIR)test/make-tests.sh
	
	(cd $(BIN_DIR)test ; rm AllTests.c ; ./make-tests.sh > AllTests.c)

	$(CC) $(DEBUGFLAGS) \
	-o $(BIN_DIR)/test/test.o \
	-I$(CUTEST_DIR) \
	-I$(INCLUDE_DIR) \
	-I$(TESTS_DIR) \
	$(TESTS) \
	$(CUTEST_DIR)CuTest.c \
	$(BIN_DIR)test/AllTests.c \
	$(OBJS)

	chmod +x $(BIN_DIR)/test/test.o

.PHONY:
test: mktest
	./$(BIN_DIR)/test/test.o

debug: mktest
	cp $(BIN_DIR)/test/test.o debug

.PHONY:
changecfg:
	$(eval CONFIG := $(TESTS_DIR)config)
	$(eval CFLAGS := $(DEBUGFLAGS))
