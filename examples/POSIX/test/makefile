TARGET_EXEC ?= sched_test

BUILD_DIR ?= ./build
SRC_DIR ?= ./src
SCHED_DIR ?= ../../../src/scheduler

INC_DIRS := $(shell find $(SRC_DIR) -type d) $(shell find $(SCHED_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CFLAGS ?= $(INC_FLAGS) -MMD -MP

all:
	mkdir -p $(BUILD_DIR)
	# Build the Scheduler Library
	$(CC) $(CFLAGS) -c $(SCHED_DIR)/scheduler.c -o $(BUILD_DIR)/scheduler.o
	# Build the Application
	$(CC) $(CFLAGS) -c $(SRC_DIR)/task_time.c -o $(BUILD_DIR)/task_time.o
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $(BUILD_DIR)/main.o
	# Build Executable
	$(CC) $(BUILD_DIR)/scheduler.o $(BUILD_DIR)/main.o $(BUILD_DIR)/task_time.o -o $(BUILD_DIR)/$(TARGET_EXEC) $(LDFLAGS)
	
	
.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p

