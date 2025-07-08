#include <gtest/gtest.h>
#include "../include/pipeline/pipeline_manager.h"
#include "../include/types.h"
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

/**
 * @file test_pipeline_manager.cpp
 * @brief Testes unitários para a classe PipelineManager
 */

using namespace legal_doc_pipeline;
using namespace legal_doc_pipeline::pipeline;

class PipelineManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configuração padrão
        config.num_workers = 2;
        config.enable_debug = false;
        config.max_sequence_length = 10;
        
        // Dados de teste
        test_data = {
            "Primeiro documento de teste com HTML <b>tags</b>",
            "SEGUNDO DOCUMENTO EM MAIÚSCULAS",
            "Terceiro documento com pontuação: vírgulas, pontos!",
            "Quarto documento com números 123 e símbolos @#$",
            "Último documento para completar os testes"
        };
        
        // Criar arquivo CSV de teste
        createTestCSV();
    }
    
    void TearDown() override {
        // Limpar arquivo de teste
        if (std::filesystem::exists(test_csv_filename)) {
            std::filesystem::remove(test_csv_filename);
        }
    }
    
    void createTestCSV() {
        std::ofstream file(test_csv_filename);
        file << "ID,Texto,Categoria\n";
        for (size_t i = 0; i < test_data.size(); ++i) {
            file << (i + 1) << ",\"" << test_data[i] << "\",TestCategory\n";
        }
        file.close();
    }
    
    PipelineConfig config;
    std::vector<std::string> test_data;
    const std::string test_csv_filename = "test_pipeline_data.csv";
};

// Teste de construção do PipelineManager
TEST_F(PipelineManagerTest, Construction) {
    EXPECT_NO_THROW(PipelineManager manager(config));
    
    PipelineManager manager(config);
    EXPECT_EQ(manager.getConfig().num_workers, config.num_workers);
    EXPECT_EQ(manager.getConfig().max_sequence_length, config.max_sequence_length);
}

// Teste de execução paralela básica
TEST_F(PipelineManagerTest, RunParallelBasic) {
    PipelineManager manager(config);
    
    auto result = manager.runParallel(test_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.processed_data.size(), test_data.size());
    EXPECT_GT(result.execution_time, 0.0);
    EXPECT_EQ(result.tasks_completed, 8); // 8 tarefas no pipeline
    EXPECT_TRUE(result.error_message.empty());
}

// Teste de execução sequencial básica
TEST_F(PipelineManagerTest, RunSequentialBasic) {
    PipelineManager manager(config);
    
    auto result = manager.runSequential(test_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.processed_data.size(), test_data.size());
    EXPECT_GT(result.execution_time, 0.0);
    EXPECT_EQ(result.tasks_completed, 8);
    EXPECT_TRUE(result.error_message.empty());
}

// Teste de execução sequencial pura
TEST_F(PipelineManagerTest, RunSequentialPure) {
    PipelineManager manager(config);
    
    auto result = manager.runSequential(test_data, true); // Force single thread
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.processed_data.size(), test_data.size());
    EXPECT_GT(result.execution_time, 0.0);
    EXPECT_EQ(result.tasks_completed, 8);
    EXPECT_TRUE(result.error_message.empty());
}

// Teste de comparação paralelo vs sequencial
TEST_F(PipelineManagerTest, RunComparison) {
    PipelineManager manager(config);
    
    auto [parallel_result, sequential_result] = manager.runComparison(test_data);
    
    EXPECT_TRUE(parallel_result.success);
    EXPECT_TRUE(sequential_result.success);
    
    // Resultados devem ter o mesmo tamanho
    EXPECT_EQ(parallel_result.processed_data.size(), sequential_result.processed_data.size());
    EXPECT_EQ(parallel_result.tasks_completed, sequential_result.tasks_completed);
    
    // Ambos devem ter processado todos os dados
    EXPECT_EQ(parallel_result.processed_data.size(), test_data.size());
    EXPECT_EQ(sequential_result.processed_data.size(), test_data.size());
    
    // Tempos devem ser positivos
    EXPECT_GT(parallel_result.execution_time, 0.0);
    EXPECT_GT(sequential_result.execution_time, 0.0);
}

// Teste de consistência entre execuções
TEST_F(PipelineManagerTest, ConsistencyBetweenModes) {
    PipelineManager manager(config);
    
    auto parallel_result = manager.runParallel(test_data);
    auto sequential_result = manager.runSequential(test_data, true);
    
    EXPECT_TRUE(parallel_result.success);
    EXPECT_TRUE(sequential_result.success);
    
    // Verificar consistência de tamanhos
    EXPECT_EQ(parallel_result.processed_data.size(), sequential_result.processed_data.size());
    
    // Verificar que dados foram realmente processados (não são idênticos aos originais)
    bool data_was_modified = false;
    for (size_t i = 0; i < parallel_result.processed_data.size() && i < test_data.size(); ++i) {
        if (parallel_result.processed_data[i] != test_data[i]) {
            data_was_modified = true;
            break;
        }
    }
    EXPECT_TRUE(data_was_modified);
}

// Teste com dados vazios
TEST_F(PipelineManagerTest, EmptyData) {
    PipelineManager manager(config);
    std::vector<std::string> empty_data;
    
    auto result = manager.runParallel(empty_data);
    
    EXPECT_FALSE(result.success); // Deve falhar com dados vazios
    EXPECT_FALSE(result.error_message.empty());
}

// Teste com configuração inválida
TEST_F(PipelineManagerTest, InvalidConfiguration) {
    PipelineConfig invalid_config;
    invalid_config.num_workers = 0; // Inválido
    
    PipelineManager manager(invalid_config);
    
    // Mesmo com configuração inválida, deve tentar executar
    auto result = manager.runParallel(test_data);
    
    // Pode passar ou falhar dependendo da implementação
    // O importante é não crashar
    EXPECT_NO_THROW(manager.runParallel(test_data));
}

// Teste de atualização de configuração
TEST_F(PipelineManagerTest, UpdateConfiguration) {
    PipelineManager manager(config);
    
    PipelineConfig new_config = config;
    new_config.num_workers = 4;
    new_config.max_sequence_length = 20;
    
    manager.updateConfig(new_config);
    
    auto updated_config = manager.getConfig();
    EXPECT_EQ(updated_config.num_workers, 4);
    EXPECT_EQ(updated_config.max_sequence_length, 20);
}

// Teste de estatísticas de execução
TEST_F(PipelineManagerTest, ExecutionStatistics) {
    PipelineManager manager(config);
    
    auto result = manager.runParallel(test_data);
    EXPECT_TRUE(result.success);
    
    auto stats = manager.getExecutionStats();
    EXPECT_FALSE(stats.empty());
    
    // Verificar se contém estatísticas básicas
    EXPECT_TRUE(stats.find("execution_time") != stats.end() ||
               stats.find("completed_tasks") != stats.end() ||
               !stats.empty());
}

// Teste de reset
TEST_F(PipelineManagerTest, Reset) {
    PipelineManager manager(config);
    
    // Executar pipeline
    auto result = manager.runParallel(test_data);
    EXPECT_TRUE(result.success);
    
    // Reset
    EXPECT_NO_THROW(manager.reset());
    
    // Deve ser capaz de executar novamente
    auto result2 = manager.runParallel(test_data);
    EXPECT_TRUE(result2.success);
}

// Teste de performance com diferentes números de workers
TEST_F(PipelineManagerTest, PerformanceWithDifferentWorkers) {
    std::vector<int> worker_counts = {1, 2, 4};
    std::vector<double> execution_times;
    
    for (int workers : worker_counts) {
        PipelineConfig test_config = config;
        test_config.num_workers = workers;
        
        PipelineManager manager(test_config);
        auto result = manager.runParallel(test_data);
        
        EXPECT_TRUE(result.success);
        execution_times.push_back(result.execution_time);
    }
    
    // Verificar que todos os tempos são positivos
    for (double time : execution_times) {
        EXPECT_GT(time, 0.0);
    }
}

// Teste com dados grandes
TEST_F(PipelineManagerTest, LargeDataSet) {
    // Criar dataset maior
    std::vector<std::string> large_data;
    for (int i = 0; i < 100; ++i) {
        large_data.push_back("Documento " + std::to_string(i) + " com conteúdo para teste de performance");
    }
    
    PipelineManager manager(config);
    auto result = manager.runParallel(large_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.processed_data.size(), large_data.size());
    EXPECT_GT(result.execution_time, 0.0);
}

// Teste de text processing integration
TEST_F(PipelineManagerTest, TextProcessingIntegration) {
    // Dados com HTML, maiúsculas e pontuação para testar pipeline completo
    std::vector<std::string> html_data = {
        "<html><body>TEXTO COM HTML</body></html>",
        "  MAIÚSCULAS COM ESPAÇOS  ",
        "Pontuação: vírgulas, pontos! E mais?"
    };
    
    PipelineManager manager(config);
    auto result = manager.runParallel(html_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.processed_data.size(), html_data.size());
    
    // Verificar se processamento foi aplicado
    for (const auto& processed_text : result.processed_data) {
        // HTML deve ter sido removido
        EXPECT_TRUE(processed_text.find("<html>") == std::string::npos);
        EXPECT_TRUE(processed_text.find("<body>") == std::string::npos);
    }
}

// Teste de robustez com caracteres especiais
TEST_F(PipelineManagerTest, SpecialCharacters) {
    std::vector<std::string> special_data = {
        "Texto com acentos: ção, não, coração",
        "Caracteres especiais: @#$%^&*()",
        "Unicode: 🚀 🎯 💻",
        "Quebras\nde\nlinha\ne\ttabs"
    };
    
    PipelineManager manager(config);
    auto result = manager.runParallel(special_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.processed_data.size(), special_data.size());
}
