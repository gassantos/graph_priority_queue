cmake_minimum_required(VERSION 3.16)
project(LegalDocumentPipeline VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})

# Source files
set(LIBRARY_SOURCES
    src/types.cpp
    src/utils/csv_reader.cpp
    src/utils/timer.cpp
    src/pipeline/text_processor.cpp
    src/pipeline/pipeline_manager.cpp
    src/scheduler/workflow_scheduler.cpp
    src/tokenizer/tokenizer_wrapper.cpp
)

# Create library for the pipeline components
add_library(pipeline_lib STATIC ${LIBRARY_SOURCES})
target_link_libraries(pipeline_lib Threads::Threads)

# Main executable
add_executable(pipeline_processor main_modular.cpp)
target_link_libraries(pipeline_processor pipeline_lib)

# Debug version
add_executable(pipeline_processor_debug main_modular.cpp)
target_link_libraries(pipeline_processor_debug pipeline_lib)
target_compile_definitions(pipeline_processor_debug PRIVATE DEBUG)

# Install targets
install(TARGETS pipeline_processor
    RUNTIME DESTINATION bin
)

# Custom targets for convenience
add_custom_target(run
    COMMAND pipeline_processor
    DEPENDS pipeline_processor
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running the pipeline processor"
)

add_custom_target(run-debug
    COMMAND pipeline_processor_debug
    DEPENDS pipeline_processor_debug
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running the debug pipeline processor"
)

# Show project structure
add_custom_target(structure
    COMMAND find . -type f -name "*.h" -o -name "*.cpp" -o -name "CMakeLists.txt" | sort
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Showing project structure"
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "LegalDocumentPipeline")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Legal Document Processing Pipeline")
set(CPACK_PACKAGE_VENDOR "Legal Tech Solutions")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

include(CPack)

# Enable testing
enable_testing()

# Add a simple test (can be expanded)
add_test(
    NAME pipeline_test
    COMMAND pipeline_processor
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Print configuration information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
else()
    message(STATUS "Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()
