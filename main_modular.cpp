#include "include/pipeline/pipeline_manager.h"
#include "include/utils/csv_reader.h"
#include "include/utils/timer.h"
#include "include/types.h"
#include <iostream>
#include <algorithm>

/**
 * @file main.cpp
 * @brief Aplicação principal do pipeline de pré-processamento de documentos jurídicos
 * 
 * Este arquivo contém a função principal que coordena a execução do pipeline
 * de processamento de texto para documentos jurídicos, demonstrando tanto
 * a execução paralela quanto sequencial.
 */

using namespace legal_doc_pipeline;

/**
 * @brief Função para exibir exemplos de resultado do processamento
 * @param result Resultado do pipeline
 * @param pipeline_type Tipo do pipeline (para log)
 * @param num_examples Número de exemplos a exibir
 */
void displayResults(const legal_doc_pipeline::PipelineResult& result, 
                   const std::string& pipeline_type, 
                   size_t num_examples = 5) {
    if (!result.success) {
        std::cout << "\nErro no " << pipeline_type << ": " << result.error_message << std::endl;
        return;
    }

    std::cout << "\n--- Resultado do " << pipeline_type 
              << " (primeiras " << num_examples << " entradas) ---" << std::endl;
    
    size_t max_examples = std::min(num_examples, result.processed_data.size());
    for (size_t i = 0; i < max_examples; ++i) {
        std::string preview = result.processed_data[i];
        if (preview.length() > 150) {
            preview = preview.substr(0, 150) + "...";
        }
        std::cout << "  Entrada " << (i + 1) << ": " << preview << std::endl;
    }
}

/**
 * @brief Função para imprimir estatísticas detalhadas
 * @param parallel_result Resultado do pipeline paralelo
 * @param sequential_result Resultado do pipeline sequencial
 */
void printDetailedStats(const legal_doc_pipeline::PipelineResult& parallel_result,
                       const legal_doc_pipeline::PipelineResult& sequential_result) {
    std::cout << "\n=== ESTATÍSTICAS DETALHADAS ===" << std::endl;
    
    if (parallel_result.success) {
        std::cout << "Pipeline Paralelo:" << std::endl;
        std::cout << "  - Tarefas concluídas: " << parallel_result.tasks_completed << std::endl;
        std::cout << "  - Tempo de execução: " << parallel_result.execution_time << " segundos" << std::endl;
        std::cout << "  - Documentos processados: " << parallel_result.processed_data.size() << std::endl;
    }
    
    if (sequential_result.success) {
        std::cout << "Pipeline Sequencial:" << std::endl;
        std::cout << "  - Tarefas concluídas: " << sequential_result.tasks_completed << std::endl;
        std::cout << "  - Tempo de execução: " << sequential_result.execution_time << " segundos" << std::endl;
        std::cout << "  - Documentos processados: " << sequential_result.processed_data.size() << std::endl;
    }
    
    if (parallel_result.success && sequential_result.success) {
        double speedup = sequential_result.execution_time / parallel_result.execution_time;
        double efficiency = speedup / 4.0; // Assumindo 4 workers
        
        std::cout << "\nComparação de Performance:" << std::endl;
        std::cout << "  - Speedup: " << speedup << "x" << std::endl;
        std::cout << "  - Eficiência: " << (efficiency * 100) << "%" << std::endl;
    }
}

/**
 * @brief Função principal da aplicação
 */
int main() {
    std::cout << "=== Pipeline de Pré-processamento de Dados Jurídicos ===" << std::endl;
    std::cout << "Versão Modular - Engenharia de Software" << std::endl;

    // Configuração do pipeline
    legal_doc_pipeline::PipelineConfig config;
    config.num_workers = 4;
    config.enable_debug = false;
    config.max_sequence_length = 128;

    try {
        // 1. Carregar os dados usando o módulo de utilitários
        legal_doc_pipeline::utils::ScopedTimer loading_timer("Carregamento de dados");
        
        std::string csv_filename = "Sumarizacao_Doc_TCERJ.csv";
        std::string column_to_process = "Texto";
        
        std::cout << "\nLendo coluna '" << column_to_process 
                  << "' do arquivo '" << csv_filename << "'..." << std::endl;

        legal_doc_pipeline::utils::CsvReader csv_reader;
        
        if (!csv_reader.validateFile(csv_filename)) {
            std::cerr << "Erro: Arquivo CSV não encontrado ou ilegível: " << csv_filename << std::endl;
            return 1;
        }

        std::vector<std::string> initial_texts = csv_reader.readColumn(csv_filename, column_to_process);

        if (initial_texts.empty()) {
            std::cerr << "Erro: Nenhum dado lido ou coluna não encontrada." << std::endl;
            return 1;
        }

        std::cout << "Total de " << initial_texts.size() 
                  << " entradas lidas da coluna '" << column_to_process << "'." << std::endl;

        // 2. Executar o pipeline usando o gerenciador
        legal_doc_pipeline::pipeline::PipelineManager manager(config);

        // Executa ambos os pipelines e compara
        auto [parallel_result, sequential_result] = manager.runComparison(initial_texts);

        // 3. Exibir resultados
        #ifdef DEBUG
            displayResults(parallel_result, "Pipeline Paralelo");
            displayResults(sequential_result, "Pipeline Sequencial");
        #endif

        printDetailedStats(parallel_result, sequential_result);

        // 4. Verificar consistência dos resultados
        if (parallel_result.success && sequential_result.success) {
            bool results_match = (parallel_result.processed_data.size() == sequential_result.processed_data.size());
            
            if (results_match) {
                std::cout << "\n✓ Resultados dos pipelines são consistentes!" << std::endl;
            } else {
                std::cout << "\n⚠ Aviso: Tamanhos dos resultados diferem entre pipelines!" << std::endl;
            }
        }

        std::cout << "\n=== Execução concluída com sucesso ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Erro durante a execução: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
