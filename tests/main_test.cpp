#include <gtest/gtest.h>

/**
 * @file main_test.cpp
 * @brief Arquivo principal para execução de todos os testes unitários
 * 
 * Este arquivo inicializa o framework Google Test e executa todos os testes
 * definidos nos diferentes módulos do projeto.
 */

int main(int argc, char **argv) {
    // Inicializar Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Configurar output para ser mais verboso
    ::testing::FLAGS_gtest_output = "xml:test_results.xml";
    
    // Executar todos os testes
    return RUN_ALL_TESTS();
}
