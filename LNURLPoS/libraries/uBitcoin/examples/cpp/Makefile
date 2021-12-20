TARGET ?= main

ifeq ($(OS),Windows_NT)
EXT ?= .exe
else
EXT ?= 
endif

TARGET_EXEC ?= $(TARGET)$(EXT)

# Paths
# to make sure addprefix to LIB_DIR doesn't go out from build directory
BUILD_DIR = build
SRC_DIR = .
# uBitcoin library
LIB_DIR = ../../src

# Tools
ifeq ($(OS),Windows_NT)
TOOLCHAIN_PREFIX ?= x86_64-w64-mingw32-
MKDIR_P = mkdir
RM_R = rmdir /s /q
else
TOOLCHAIN_PREFIX ?= 
MKDIR_P = mkdir -p
RM_R = rm -r
endif

# compilers
CC := $(TOOLCHAIN_PREFIX)gcc
CXX := $(TOOLCHAIN_PREFIX)g++

# main.cpp
CXX_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
C_SOURCES =

# uBitcoin sources
CXX_SOURCES += $(wildcard $(LIB_DIR)/*.cpp)
C_SOURCES += $(wildcard $(LIB_DIR)/utility/trezor/*.c) \
			$(wildcard $(LIB_DIR)/utility/*.c) \
			$(wildcard $(LIB_DIR)/*.c)

# include lib path, don't use mbed or arduino config (-DUSE_STDONLY), debug symbols, all warnings as errors
FLAGS = -I$(LIB_DIR) -g -Wall -Werror -ldl
CFLAGS = $(FLAGS)
CPPFLAGS = $(FLAGS) -DUSE_STDONLY

OBJS = $(patsubst $(SRC_DIR)/%, $(BUILD_DIR)/src/%.o, \
		$(patsubst $(LIB_DIR)/%, $(BUILD_DIR)/lib/%.o, \
		$(C_SOURCES) $(CXX_SOURCES)))

vpath %.cpp $(SRC_DIR)
vpath %.cpp $(LIB_DIR)
vpath %.c $(LIB_DIR)

.PHONY: clean all run

all: $(BUILD_DIR)/$(TARGET_EXEC)

run: $(BUILD_DIR)/$(TARGET_EXEC)
	$(BUILD_DIR)/$(TARGET_EXEC)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) $(CPPFLAGS) -o $@

# lib c sources
$(BUILD_DIR)/lib/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

# lib cpp sources
$(BUILD_DIR)/lib/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) -c $(CPPFLAGS) $< -o $@

# cpp sources
$(BUILD_DIR)/src/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) -c $(CPPFLAGS) $< -o $@

clean:
	$(RM_R) $(BUILD_DIR)
