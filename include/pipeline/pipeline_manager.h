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
         * @return Resultado da execução
         */
        PipelineResult runSequential(const std::vector<std::string>& input_data);

        /**
         * @brief Executa ambos os modos e compara performance
         * @param input_data Dados de entrada
         * @return Par com resultados (paralelo, sequencial)
         */
        std::pair<PipelineResult, PipelineResult> runComparison(
            const std::vector<std::string>& input_data
        );

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
