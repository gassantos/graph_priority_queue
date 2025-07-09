#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include "../types.h"
#include "../utils/timer.h"
#include <memory>
#include <map>
#include <vector>
#include <string>

/**
 * @file pipeline_manager.h
 * @brief Gerenciador principal do pipeline
 * 
 * Esta classe coordena a execução de todo o pipeline de processamento,
 * tanto em modo sequencial quanto paralelo.
 */

namespace legal_doc_pipeline {

// Forward declarations
namespace scheduler { class WorkflowScheduler; }

namespace pipeline {

    /**
     * @brief Classe principal para gerenciamento do pipeline
     */
    class PipelineManager {
    private:
        PipelineConfig config;                                      ///< Configuração do pipeline
        std::unique_ptr<scheduler::WorkflowScheduler> scheduler;    ///< Scheduler para execução paralela
        utils::Timer timer;                                         ///< Timer para medição de performance
        
        // Estatísticas de execução
        mutable double last_parallel_time = 0.0;                   ///< Tempo da última execução paralela
        mutable double last_sequential_time = 0.0;                 ///< Tempo da última execução sequencial
        mutable double last_partitioned_time = 0.0;                ///< Tempo da última execução paralela particionada

        /**
         * @brief Configura as tarefas no scheduler
         * @param scheduler_ptr Ponteiro para o scheduler
         */
        void setupTasks(scheduler::WorkflowScheduler* scheduler_ptr);

        /**
         * @brief Configura as dependências entre tarefas
         * @param scheduler_ptr Ponteiro para o scheduler
         */
        void setupDependencies(scheduler::WorkflowScheduler* scheduler_ptr);

        /**
         * @brief Valida os dados de entrada
         * @param input_data Dados a serem validados
         * @return true se os dados são válidos
         */
        bool validateInput(const std::vector<std::string>& input_data);

        /**
         * @brief Preparar dados para processamento
         * @param input_data Dados de entrada
         * @return Dados preparados para processamento
         */
        std::vector<std::string> prepareData(const std::vector<std::string>& input_data);

    public:
        /**
         * @brief Construtor com configuração
         * @param config Configuração do pipeline
         */
        explicit PipelineManager(const PipelineConfig& config = PipelineConfig{});

        /**
         * @brief Destrutor
         */
        ~PipelineManager();

        /**
         * @brief Executa o pipeline em modo paralelo
         * @param input_data Dados de entrada
         * @return Resultado da execução
         */
        PipelineResult runParallel(const std::vector<std::string>& input_data);

        /**
         * @brief Executa o pipeline em modo sequencial
         * @param input_data Dados de entrada
         * @param force_single_thread Força execução em thread única
         * @return Resultado da execução
         */
        PipelineResult runSequential(const std::vector<std::string>& input_data, 
                                    bool force_single_thread = true);

        /**
         * @brief Executa ambos os modos e compara performance
         * @param input_data Dados de entrada
         * @return Par com resultados (paralelo, sequencial)
         */
        std::pair<PipelineResult, PipelineResult> runComparison(
            const std::vector<std::string>& input_data
        );

        /**
         * @brief Executa todos os três modos e compara performance
         * @param input_data Dados de entrada
         * @return Estrutura com todos os resultados de comparação
         */
        ComparisonResult runFullComparison(const std::vector<std::string>& input_data);

        /**
         * @brief Executa o pipeline em modo paralelo com particionamento de dados
         * @param input_data Dados de entrada
         * @return Resultado da execução
         */
        PipelineResult runParallelPartitioned(const std::vector<std::string>& input_data);

        /**
         * @brief Calcula o tamanho ideal de chunk para particionamento
         * @param total_size Tamanho total dos dados
         * @param num_workers Número de workers disponíveis
         * @return Tamanho ideal do chunk
         */
        size_t calculateOptimalChunkSize(size_t total_size, size_t num_workers);

        /**
         * @brief Particiona os dados em chunks para processamento paralelo
         * @param data Dados a serem particionados
         * @param chunk_size Tamanho de cada chunk
         * @return Vector de chunks
         */
        std::vector<std::vector<std::string>> partitionData(
            const std::vector<std::string>& data, size_t chunk_size);

        /**
         * @brief Processa um chunk de dados sequencialmente
         * @param chunk_data Dados do chunk
         * @param chunk_id ID do chunk para debug
         * @return Dados processados
         */
        std::vector<std::string> processChunkSequentially(
            const std::vector<std::string>& chunk_data, size_t chunk_id);

        /**
         * @brief Reconstrói os dados processados a partir dos chunks
         * @param processed_chunks Chunks processados
         * @return Dados reconstruídos na ordem original
         */
        std::vector<std::string> mergeProcessedChunks(
            const std::vector<std::vector<std::string>>& processed_chunks);

        /**
         * @brief Obtém a configuração atual
         * @return Referência para a configuração
         */
        const PipelineConfig& getConfig() const;

        /**
         * @brief Atualiza a configuração
         * @param new_config Nova configuração
         */
        void updateConfig(const PipelineConfig& new_config);

        /**
         * @brief Obtém estatísticas da última execução
         * @return Mapa com estatísticas detalhadas
         */
        std::map<std::string, double> getExecutionStats() const;

        /**
         * @brief Limpa recursos e reinicia o estado
         */
        void reset();

        // Desabilita cópia e atribuição
        PipelineManager(const PipelineManager&) = delete;
        PipelineManager& operator=(const PipelineManager&) = delete;

        // Permite movimentação
        PipelineManager(PipelineManager&&) = default;
        PipelineManager& operator=(PipelineManager&&) = default;
    };

} // namespace pipeline
} // namespace legal_doc_pipeline

#endif // PIPELINE_MANAGER_H
