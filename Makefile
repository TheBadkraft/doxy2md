# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -Iinclude -std=c99
LDFLAGS = -lsigcore  # Link against sigcore library

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Executable
TARGET = $(BIN_DIR)/doxy

# Install path
INSTALL_DIR = ~/bin

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile source file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Install to ~/bin
install: $(TARGET)
	@mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/

# Clean build artifacts
clean:
	rm -f $(BUILD_DIR)/* $(BIN_DIR)/*

# Run the executable (optional)
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean install run
