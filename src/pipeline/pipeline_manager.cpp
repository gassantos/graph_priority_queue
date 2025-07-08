#include "../../include/pipeline/pipeline_manager.h"
#include "../../include/pipeline/text_processor.h"
#include "../../include/scheduler/workflow_scheduler.h"
#include "../../include/utils/timer.h"
#include <iostream>
#include <memory>

namespace legal_doc_pipeline {
namespace pipeline {

    PipelineManager::PipelineManager(const PipelineConfig& config) 
        : config(config), scheduler(std::make_unique<scheduler::WorkflowScheduler>()) {}

    PipelineManager::~PipelineManager() = default;

    PipelineResult PipelineManager::runParallel(const std::vector<std::string>& input_data) {
        PipelineResult result;
        result.success = false;

        if (!validateInput(input_data)) {
            result.error_message = "Dados de entrada inválidos";
            return result;
        }

        try {
            std::cout << "\n--- Iniciando Pipeline Paralelo com " << config.num_workers 
                      << " threads de worker ---" << std::endl;

            timer.start();

            // Prepara dados
            std::vector<std::string> processed_data = prepareData(input_data);

            // Configura o scheduler
            scheduler->clear();
            setupTasks(scheduler.get());
            setupDependencies(scheduler.get());

            // Executa o pipeline
            bool success = scheduler->run(processed_data, config.num_workers);

            timer.stop();

            if (success) {
                result.processed_data = scheduler->getProcessedData();
                result.execution_time = timer.getElapsedSeconds();
                result.tasks_completed = scheduler->getExecutionStats().at("completed_tasks");
                result.success = true;

                std::cout << "--- Pipeline Paralelo Concluído ---" << std::endl;
                std::cout << "Tempo total de execução (paralelo): " << timer.getElapsedString() << std::endl;
            } else {
                result.error_message = "Falha na execução do pipeline paralelo";
            }

        } catch (const std::exception& e) {
            timer.stop();
            result.error_message = "Exceção durante execução paralela: " + std::string(e.what());
        }

        return result;
    }

    PipelineResult PipelineManager::runSequential(const std::vector<std::string>& input_data, 
                                                 bool force_single_thread) {
        PipelineResult result;
        result.success = false;

        if (!validateInput(input_data)) {
            result.error_message = "Dados de entrada inválidos";
            return result;
        }

        try {
            std::cout << "\n--- Iniciando Pipeline Sequencial ";
            if (force_single_thread) {
                std::cout << "(Modo Thread Única) ";
            }
            std::cout << "---" << std::endl;

            timer.start();

            // Prepara dados
            std::vector<std::string> processed_data = prepareData(input_data);

            if (force_single_thread) {
                // Execução verdadeiramente sequencial - uma tarefa de cada vez, sem paralelismo
                size_t task_count = 0;

                TextProcessor::cleanTextSequential(processed_data);
                task_count++;
                std::cout << "Tarefa 'CleanText' finalizada! Total concluídas: " << task_count << std::endl;

                TextProcessor::normalizeTextSequential(processed_data);
                task_count++;
                std::cout << "Tarefa 'NormalizeText' finalizada! Total concluídas: " << task_count << std::endl;

                TextProcessor::wordTokenizationSequential(processed_data);
                task_count++;
                std::cout << "Tarefa 'WordTokenization' finalizada! Total concluídas: " << task_count << std::endl;

                TextProcessor::bpeTokenization(processed_data);
                task_count++;
                std::cout << "Tarefa 'BPETokenization' finalizada! Total concluídas: " << task_count << std::endl;

                TextProcessor::partitionTokens(processed_data, config.max_sequence_length);
                task_count++;
                std::cout << "Tarefa 'PartitionTokens' finalizada! Total concluídas: " << task_count << std::endl;

                TextProcessor::addSpecialTokens(processed_data);
                task_count++;
                std::cout << "Tarefa 'AddSpecialTokens' finalizada! Total concluídas: " << task_count << std::endl;

                TextProcessor::tokensToIndices(processed_data);
                task_count++;
                std::cout << "Tarefa 'TokensToIndices' finalizada! Total concluídas: " << task_count << std::endl;

                TextProcessor::generateEmbeddings(processed_data);
                task_count++;
                std::cout << "Tarefa 'GenerateEmbeddings' finalizada! Total concluídas: " << task_count << std::endl;

                timer.stop();

                result.processed_data = processed_data;
                result.execution_time = timer.getElapsedSeconds();
                result.tasks_completed = task_count;
                result.success = true;

                std::cout << "--- Pipeline Sequencial Concluído ---" << std::endl;
                std::cout << "Total de tarefas concluídas (sequencial): " << task_count << std::endl;
                std::cout << "Tempo total de execução (sequencial): " << timer.getElapsedString() << std::endl;
            } else {
                // Modo sequencial anterior (pode usar scheduler com 1 worker)
                // Criar um scheduler temporário com configuração sequencial
                auto sequential_scheduler = std::make_unique<scheduler::WorkflowScheduler>();
                sequential_scheduler->clear();
                setupTasks(sequential_scheduler.get());
                setupDependencies(sequential_scheduler.get());

                // Executa com apenas 1 worker
                bool success = sequential_scheduler->run(processed_data, 1);

                timer.stop();

                if (success) {
                    result.processed_data = sequential_scheduler->getProcessedData();
                    result.execution_time = timer.getElapsedSeconds();
                    result.tasks_completed = sequential_scheduler->getExecutionStats().at("completed_tasks");
                    result.success = true;

                    std::cout << "--- Pipeline Sequencial Concluído ---" << std::endl;
                    std::cout << "Tempo total de execução (sequencial): " << timer.getElapsedString() << std::endl;
                } else {
                    result.error_message = "Falha na execução do pipeline sequencial";
                }
            }

        } catch (const std::exception& e) {
            timer.stop();
            result.error_message = "Exceção durante execução sequencial: " + std::string(e.what());
        }

        return result;
    }

    std::pair<PipelineResult, PipelineResult> PipelineManager::runComparison(
        const std::vector<std::string>& input_data) {
        
        auto parallel_result = runParallel(input_data);
        auto sequential_result = runSequential(input_data);

        // Imprimir comparação
        if (parallel_result.success && sequential_result.success) {
            std::cout << "\n=== COMPARAÇÃO DE PERFORMANCE ===" << std::endl;
            std::cout << "Tempo Paralelo:   " << parallel_result.execution_time << " segundos" << std::endl;
            std::cout << "Tempo Sequencial: " << sequential_result.execution_time << " segundos" << std::endl;
            
            if (sequential_result.execution_time > 0) {
                double speedup = sequential_result.execution_time / parallel_result.execution_time;
                std::cout << "Speedup:          " << speedup << "x" << std::endl;
            }
        }

        return std::make_pair(parallel_result, sequential_result);
    }

    void PipelineManager::setupTasks(scheduler::WorkflowScheduler* scheduler_ptr) {
        // Adiciona as tarefas com suas prioridades
        scheduler_ptr->addTask(Task("CleanText", TaskType::TEXT_CLEANING, 10, 
                                   [](std::vector<std::string>& texts) { 
                                       TextProcessor::cleanText(texts); 
                                   }));

        scheduler_ptr->addTask(Task("NormalizeText", TaskType::NORMALIZATION, 20, 
                                   [](std::vector<std::string>& texts) { 
                                       TextProcessor::normalizeText(texts); 
                                   }));

        scheduler_ptr->addTask(Task("WordTokenization", TaskType::WORD_TOKENIZATION, 30, 
                                   [](std::vector<std::string>& texts) { 
                                       TextProcessor::wordTokenization(texts); 
                                   }));

        scheduler_ptr->addTask(Task("BPETokenization", TaskType::BPE_TOKENIZATION, 40, 
                                   [](std::vector<std::string>& texts) { 
                                       TextProcessor::bpeTokenization(texts); 
                                   }));

        scheduler_ptr->addTask(Task("PartitionTokens", TaskType::PARTITION_TOKENS, 50, 
                                   [this](std::vector<std::string>& texts) { 
                                       TextProcessor::partitionTokens(texts, config.max_sequence_length); 
                                   }));

        scheduler_ptr->addTask(Task("AddSpecialTokens", TaskType::ADD_SPECIAL_TOKENS, 60, 
                                   [](std::vector<std::string>& texts) { 
                                       TextProcessor::addSpecialTokens(texts); 
                                   }));

        scheduler_ptr->addTask(Task("TokensToIndices", TaskType::TOKENS_TO_INDICES, 70, 
                                   [](std::vector<std::string>& texts) { 
                                       TextProcessor::tokensToIndices(texts); 
                                   }));

        scheduler_ptr->addTask(Task("GenerateEmbeddings", TaskType::GENERATE_EMBEDDINGS, 80, 
                                   [](std::vector<std::string>& texts) { 
                                       TextProcessor::generateEmbeddings(texts); 
                                   }));
    }

    void PipelineManager::setupDependencies(scheduler::WorkflowScheduler* scheduler_ptr) {
        // Define as dependências conforme o grafo
        scheduler_ptr->addDependency("NormalizeText", "CleanText");
        scheduler_ptr->addDependency("WordTokenization", "NormalizeText");
        scheduler_ptr->addDependency("BPETokenization", "WordTokenization");
        scheduler_ptr->addDependency("PartitionTokens", "BPETokenization");
        scheduler_ptr->addDependency("AddSpecialTokens", "PartitionTokens");
        scheduler_ptr->addDependency("TokensToIndices", "AddSpecialTokens");
        scheduler_ptr->addDependency("GenerateEmbeddings", "TokensToIndices");
    }

    bool PipelineManager::validateInput(const std::vector<std::string>& input_data) {
        if (input_data.empty()) {
            std::cerr << "Erro: Dados de entrada vazios" << std::endl;
            return false;
        }

        // Verifica se há dados não vazios
        bool has_valid_data = false;
        for (const auto& text : input_data) {
            if (!text.empty()) {
                has_valid_data = true;
                break;
            }
        }

        if (!has_valid_data) {
            std::cerr << "Erro: Todos os dados de entrada estão vazios" << std::endl;
            return false;
        }

        return true;
    }

    std::vector<std::string> PipelineManager::prepareData(const std::vector<std::string>& input_data) {
        // Por enquanto, apenas copia os dados
        // Aqui poderia haver pré-processamento adicional se necessário
        return input_data;
    }

    const PipelineConfig& PipelineManager::getConfig() const {
        return config;
    }

    void PipelineManager::updateConfig(const PipelineConfig& new_config) {
        config = new_config;
    }

    std::map<std::string, double> PipelineManager::getExecutionStats() const {
        std::map<std::string, double> stats;
        stats["last_execution_time"] = timer.getElapsedSeconds();
        
        if (scheduler) {
            auto scheduler_stats = scheduler->getExecutionStats();
            stats["completed_tasks"] = static_cast<double>(scheduler_stats["completed_tasks"]);
            stats["total_tasks"] = static_cast<double>(scheduler_stats["total_tasks"]);
        }
        
        return stats;
    }

    void PipelineManager::reset() {
        if (scheduler) {
            scheduler->clear();
        }
        timer.reset();
    }

} // namespace pipeline
} // namespace legal_doc_pipeline
