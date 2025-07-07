# Makefile for Legal Document Processing Pipeline
# Modular C++ Project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
DEBUG_FLAGS = -g -DDEBUG
INCLUDE_DIRS = -I.

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(SRC_DIR)/types.cpp \
          $(SRC_DIR)/utils/csv_reader.cpp \
          $(SRC_DIR)/utils/timer.cpp \
          $(SRC_DIR)/pipeline/text_processor.cpp \
          $(SRC_DIR)/pipeline/pipeline_manager.cpp \
          $(SRC_DIR)/scheduler/workflow_scheduler.cpp \
          $(SRC_DIR)/tokenizer/tokenizer_wrapper.cpp

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Executables
TARGET = $(BIN_DIR)/pipeline_processor
TARGET_DEBUG = $(BIN_DIR)/pipeline_processor_debug
TARGET_LEGACY = $(BIN_DIR)/pipeline_processor_legacy

# Default target
all: $(TARGET)

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET_DEBUG)

# Legacy version (original main.cpp)
legacy: $(TARGET_LEGACY)

# Create directories
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# Create subdirectories for object files
$(BUILD_DIR)/utils $(BUILD_DIR)/pipeline $(BUILD_DIR)/scheduler $(BUILD_DIR)/tokenizer: | $(BUILD_DIR)
	mkdir -p $@

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/utils $(BUILD_DIR)/pipeline $(BUILD_DIR)/scheduler $(BUILD_DIR)/tokenizer
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Link modular version
$(TARGET): $(OBJECTS) main_modular.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) $(OBJECTS) main_modular.cpp -o $@

# Link debug version
$(TARGET_DEBUG): $(OBJECTS) main_modular.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(INCLUDE_DIRS) $(OBJECTS) main_modular.cpp -o $@

# Link legacy version
$(TARGET_LEGACY): main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) main.cpp $(SRC_DIR)/tokenizer/tokenizer_wrapper.cpp -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Run the modular version
run: $(TARGET)
	./$(TARGET)

# Run the debug version
run-debug: $(TARGET_DEBUG)
	./$(TARGET_DEBUG)

# Run the legacy version
run-legacy: $(TARGET_LEGACY)
	./$(TARGET_LEGACY)

# Install (copy to system location - optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/pipeline_processor

# Show project structure
structure:
	@echo "Project Structure:"
	@find . -type f -name "*.h" -o -name "*.cpp" -o -name "Makefile" | sort

# Show help
help:
	@echo "Available targets:"
	@echo "  all         - Build modular version (default)"
	@echo "  debug       - Build debug version with DEBUG flag"
	@echo "  legacy      - Build legacy version (original main.cpp)"
	@echo "  run         - Build and run modular version"
	@echo "  run-debug   - Build and run debug version"
	@echo "  run-legacy  - Build and run legacy version"
	@echo "  clean       - Remove all build files"
	@echo "  install     - Install to system (requires sudo)"
	@echo "  structure   - Show project file structure"
	@echo "  help        - Show this help message"

# Performance test
performance-test: $(TARGET) $(TARGET_LEGACY)
	@echo "=== Performance Comparison ==="
	@echo "Running modular version..."
	@time ./$(TARGET)
	@echo ""
	@echo "Running legacy version..."
	@time ./$(TARGET_LEGACY)

# Dependency tracking
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/utils $(BUILD_DIR)/pipeline $(BUILD_DIR)/scheduler $(BUILD_DIR)/tokenizer
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -MM -MT $(BUILD_DIR)/$*.o $< > $@

# Phony targets
.PHONY: all debug legacy clean run run-debug run-legacy install structure help performance-test

# Special targets
.DEFAULT_GOAL := all
.SUFFIXES:
