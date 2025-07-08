# Makefile for Legal Document Processing Pipeline

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
DEBUG_FLAGS = -g -DDEBUG
LDFLAGS = 
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

# Test files
TEST_SOURCES = tests/test_csv_reader.cpp \
               tests/test_text_processor.cpp \
               tests/test_workflow_scheduler.cpp \
               tests/test_pipeline_manager.cpp \
               tests/main_test.cpp

# Executables
TARGET = $(BIN_DIR)/pipeline_processor
TARGET_DEBUG = $(BIN_DIR)/pipeline_processor_debug
TARGET_TESTS = $(BIN_DIR)/run_tests

# Default target
all: $(TARGET)

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET_DEBUG)

# Test build
tests: $(TARGET_TESTS)

# Create directories
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# Create subdirectories for object files
$(BUILD_DIR)/utils $(BUILD_DIR)/pipeline $(BUILD_DIR)/scheduler $(BUILD_DIR)/tokenizer: | $(BUILD_DIR)
	mkdir -p $@

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/utils $(BUILD_DIR)/pipeline $(BUILD_DIR)/scheduler $(BUILD_DIR)/tokenizer
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Link version
$(TARGET): $(OBJECTS) main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) $(OBJECTS) main.cpp $(LDFLAGS) -o $@

# Link debug version
$(TARGET_DEBUG): $(OBJECTS) main.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(INCLUDE_DIRS) $(OBJECTS) main.cpp $(LDFLAGS) -o $@

# Link test version
$(TARGET_TESTS): $(OBJECTS) $(TEST_SOURCES) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) $(OBJECTS) $(TEST_SOURCES) $(LDFLAGS) -lgtest -lgtest_main -pthread -o $@
	@echo "Test binary created with flags: $(CXXFLAGS) $(LDFLAGS)"
	@echo "Checking if binary is instrumented:"
	@strings $@ | grep -q "__gcov" && echo "Binary has gcov instrumentation" || echo "WARNING: Binary may not have gcov instrumentation"

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Run the version
run: $(TARGET)
	./$(TARGET)

# Run the debug version
run-debug: $(TARGET_DEBUG)
	./$(TARGET_DEBUG)

# Run tests
run-tests: $(TARGET_TESTS)
	./$(TARGET_TESTS)

# Shorthand for running tests
test: run-tests

# Coverage build targets
coverage: clean-coverage build-coverage run-tests-coverage

clean-coverage:
	rm -f *.gcda *.gcno build/*.gcda build/*.gcno src/**/*.gcda src/**/*.gcno
	rm -f coverage.info coverage.info.cleaned coverage.xml
	rm -rf coverage-html

build-coverage:
	@echo "Building tests with coverage flags..."
	# Force rebuild of all object files with coverage flags
	rm -f $(OBJECTS) $(TARGET_TESTS)
	$(MAKE) tests \
		CXXFLAGS="-std=c++17 -Wall -Wextra -g --coverage -O0 -fprofile-arcs -ftest-coverage" \
		LDFLAGS="--coverage"

run-tests-coverage:
	@echo "Running tests to generate coverage data..."
	@echo "Test binary location: $(TARGET_TESTS)"
	@if [ ! -f "$(TARGET_TESTS)" ]; then echo "ERROR: Test binary not found!"; exit 1; fi
	./$(TARGET_TESTS) --gtest_output=xml:test-results-coverage.xml
	@echo "Coverage data files generated:"
	@echo "=== .gcda files ==="
	@find . -name "*.gcda" -type f 2>/dev/null | head -10 || echo "No .gcda files found"
	@echo "=== .gcno files ==="
	@find . -name "*.gcno" -type f 2>/dev/null | head -10 || echo "No .gcno files found"

# Backward compatibility
tests-coverage: build-coverage

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
	@echo "  all         - Build version (default)"
	@echo "  debug       - Build debug version with DEBUG flag"
	@echo "  tests       - Build test executable"
	@echo "  run         - Build and run version"
	@echo "  run-debug   - Build and run debug version"
	@echo "  run-tests   - Build and run tests"
	@echo "  test        - Build and run tests (shorthand)"
	@echo "  coverage    - Build and run tests with coverage"
	@echo "  clean       - Remove all build files"
	@echo "  clean-coverage - Remove coverage data files"
	@echo "  install     - Install to system (requires sudo)"
	@echo "  structure   - Show project file structure"
	@echo "  help        - Show this help message"


# Dependency tracking
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/utils $(BUILD_DIR)/pipeline $(BUILD_DIR)/scheduler $(BUILD_DIR)/tokenizer
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -MM -MT $(BUILD_DIR)/$*.o $< > $@

# Phony targets
.PHONY: all debug tests clean run run-debug run-tests test install structure help coverage clean-coverage tests-coverage run-tests-coverage

# Special targets
.DEFAULT_GOAL := all
.SUFFIXES:
