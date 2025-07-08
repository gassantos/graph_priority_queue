cmake_minimum_required(VERSION 3.16)
project(legal_doc_pipeline_tests)

# Configurações do C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Flags de compilação
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -DDEBUG")

# Encontrar Google Test
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Incluir diretórios
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${GTEST_INCLUDE_DIRS})

# Arquivos fonte do projeto principal (excluindo main.cpp e main_modular.cpp)
set(PROJECT_SOURCES
    ../src/types.cpp
    ../src/utils/csv_reader.cpp
    ../src/utils/timer.cpp
    ../src/pipeline/text_processor.cpp
    ../src/pipeline/pipeline_manager.cpp
    ../src/scheduler/workflow_scheduler.cpp
    ../src/tokenizer/tokenizer_wrapper.cpp
)

# Arquivos de teste
set(TEST_SOURCES
    test_csv_reader.cpp
    test_text_processor.cpp
    test_workflow_scheduler.cpp
    test_pipeline_manager.cpp
    main_test.cpp
)

# Criar executável de teste
add_executable(run_tests ${TEST_SOURCES} ${PROJECT_SOURCES})

# Linkar bibliotecas
target_link_libraries(run_tests 
    ${GTEST_LIBRARIES} 
    ${GTEST_MAIN_LIBRARIES}
    Threads::Threads
)

# Habilitar testes
enable_testing()

# Adicionar teste ao CTest
add_test(NAME AllTests COMMAND run_tests)

# Target para executar testes com saída detalhada
add_custom_target(test_verbose
    COMMAND ./run_tests --gtest_output=xml:test_results.xml
    DEPENDS run_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Target para executar testes e gerar relatório
add_custom_target(test_report
    COMMAND ./run_tests --gtest_output=xml:test_results.xml
    COMMAND echo "Testes executados. Resultados em test_results.xml"
    DEPENDS run_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
