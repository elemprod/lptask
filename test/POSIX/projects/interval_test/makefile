TARGET_EXEC ?= interval_test

BUILD_DIR ?= ./build
SRC_COMMON_DIR ?= ../../common
SRC_EXTERNAL_DIR ?= ../../external
SRC_DIR ?= ./src
SCHED_DIR ?= ../../../../src/scheduler

INC_DIRS := $(shell find $(SRC_DIR) -type d) $(shell find $(SCHED_DIR) -type d) $(shell find $(SRC_COMMON_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CFLAGS ?= $(INC_FLAGS) -MMD -MP

all:
	mkdir -p $(BUILD_DIR)
	# Build the Scheduler Library
	$(CC) $(CFLAGS) -c $(SCHED_DIR)/scheduler.c -o $(BUILD_DIR)/scheduler.o
	# Build the Platform Support
	$(CC) $(CFLAGS) -c $(SRC_COMMON_DIR)/sched_port.c -o $(BUILD_DIR)/sched_port.o
	# Build the Application
	$(CC) $(CFLAGS) -c $(SRC_COMMON_DIR)/task_time.c -o $(BUILD_DIR)/task_time.o
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $(BUILD_DIR)/main.o
	# Build the Executable
	$(CC) $(BUILD_DIR)/scheduler.o $(BUILD_DIR)/main.o $(BUILD_DIR)/task_time.o $(BUILD_DIR)/sched_port.o -o $(BUILD_DIR)/$(TARGET_EXEC) $(LDFLAGS) -lm
	
.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p

