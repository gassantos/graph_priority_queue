#include "../../include/pipeline/pipeline_manager.h"
#include "../../include/pipeline/text_processor.h"
#include "../../include/scheduler/workflow_scheduler.h"
#include "../../include/utils/timer.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <algorithm>
#include <iomanip>

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
            last_parallel_time = timer.getElapsedSeconds();

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
            last_parallel_time = timer.getElapsedSeconds();
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
                last_sequential_time = timer.getElapsedSeconds();

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
                last_sequential_time = timer.getElapsedSeconds();

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
            last_sequential_time = timer.getElapsedSeconds();
            result.error_message = "Exceção durante execução sequencial: " + std::string(e.what());
        }

        return result;
    }

    PipelineResult PipelineManager::runParallelPartitioned(const std::vector<std::string>& input_data) {
        PipelineResult result;
        result.success = false;

        if (!validateInput(input_data)) {
            result.error_message = "Dados de entrada inválidos";
            return result;
        }

        try {
            std::cout << "\n--- Iniciando Pipeline Paralelo com Particionamento de Dados ---" << std::endl;
            std::cout << "Total de documentos: " << input_data.size() << std::endl;
            std::cout << "Número de workers: " << config.num_workers << std::endl;

            timer.start();

            // Prepara dados
            std::vector<std::string> prepared_data = prepareData(input_data);
            
            // Calcula o tamanho ideal do chunk
            size_t chunk_size = calculateOptimalChunkSize(prepared_data.size(), config.num_workers);
            std::cout << "Tamanho do chunk: " << chunk_size << " documentos por worker" << std::endl;

            // Divide os dados em chunks
            std::vector<std::vector<std::string>> data_chunks = partitionData(prepared_data, chunk_size);
            std::cout << "Número de chunks criados: " << data_chunks.size() << std::endl;

            // Processa chunks em paralelo usando threads
            std::vector<std::thread> workers;
            std::vector<std::vector<std::string>> processed_chunks(data_chunks.size());
            std::vector<bool> chunk_success(data_chunks.size(), false);
            std::mutex progress_mutex;
            size_t completed_chunks = 0;

            // Lança workers para processar chunks em paralelo
            for (size_t i = 0; i < data_chunks.size(); ++i) {
                workers.emplace_back([this, i, &data_chunks, &processed_chunks, &chunk_success, 
                                   &progress_mutex, &completed_chunks]() {
                    try {
                        // Processa o chunk sequencialmente (pipeline completo)
                        processed_chunks[i] = processChunkSequentially(data_chunks[i], i);
                        chunk_success[i] = true;

                        // Update progress thread-safely
                        {
                            std::lock_guard<std::mutex> lock(progress_mutex);
                            completed_chunks++;
                            std::cout << "Chunk " << i << " completado! Progresso: " 
                                     << completed_chunks << "/" << data_chunks.size() << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::lock_guard<std::mutex> lock(progress_mutex);
                        std::cerr << "Erro no chunk " << i << ": " << e.what() << std::endl;
                        chunk_success[i] = false;
                    }
                });
            }

            // Aguarda todos os workers terminarem
            for (auto& worker : workers) {
                worker.join();
            }

            timer.stop();
            last_partitioned_time = timer.getElapsedSeconds();

            // Verifica se todos os chunks foram processados com sucesso
            bool all_success = true;
            for (bool success : chunk_success) {
                if (!success) {
                    all_success = false;
                    break;
                }
            }

            if (all_success) {
                // Reconstrói os dados processados na ordem original
                result.processed_data = mergeProcessedChunks(processed_chunks);
                result.execution_time = timer.getElapsedSeconds();
                result.tasks_completed = data_chunks.size() * 8; // 8 tarefas por chunk
                result.success = true;

                std::cout << "--- Pipeline Paralelo com Particionamento Concluído ---" << std::endl;
                std::cout << "Chunks processados com sucesso: " << data_chunks.size() << std::endl;
                std::cout << "Tempo total de execução: " << timer.getElapsedString() << std::endl;
                std::cout << "Throughput: " << (input_data.size() / timer.getElapsedSeconds()) 
                         << " documentos/segundo" << std::endl;
            } else {
                result.error_message = "Falha no processamento de um ou mais chunks";
            }

        } catch (const std::exception& e) {
            timer.stop();
            last_partitioned_time = timer.getElapsedSeconds();
            result.error_message = "Exceção durante execução paralela com particionamento: " + std::string(e.what());
        }

        return result;
    }

    std::pair<PipelineResult, PipelineResult> PipelineManager::runComparison(
        const std::vector<std::string>& input_data) {
        
        // Executa os três modos de pipeline
        std::cout << "\n🚀 INICIANDO COMPARAÇÃO COMPLETA DE PERFORMANCE 🚀" << std::endl;
        std::cout << "Testando " << input_data.size() << " documentos com " 
                  << config.num_workers << " workers disponíveis" << std::endl;
        
        auto parallel_result = runParallel(input_data);
        auto sequential_result = runSequential(input_data, true);
        auto partitioned_result = runParallelPartitioned(input_data);

        // Imprimir comparação detalhada
        if (parallel_result.success && sequential_result.success && partitioned_result.success) {
            std::cout << "\n📊 === ANÁLISE COMPARATIVA DE PERFORMANCE === 📊" << std::endl;
            std::cout << std::fixed << std::setprecision(4);
            
            std::cout << "\n⏱️  TEMPOS DE EXECUÇÃO:" << std::endl;
            std::cout << "  Pipeline Paralelo (Scheduler):     " << parallel_result.execution_time << " segundos" << std::endl;
            std::cout << "  Pipeline Sequencial (Thread Única): " << sequential_result.execution_time << " segundos" << std::endl;
            std::cout << "  Pipeline Paralelo (Particionado):   " << partitioned_result.execution_time << " segundos" << std::endl;
            
            std::cout << "\n🚀 SPEEDUPS:" << std::endl;
            if (sequential_result.execution_time > 0) {
                double speedup_scheduler = sequential_result.execution_time / parallel_result.execution_time;
                double speedup_partitioned = sequential_result.execution_time / partitioned_result.execution_time;
                
                std::cout << "  Scheduler vs Sequencial:     " << speedup_scheduler << "x";
                if (speedup_scheduler < 1.0) std::cout << " (PIOR)";
                else if (speedup_scheduler > 1.0) std::cout << " (MELHOR)";
                std::cout << std::endl;
                
                std::cout << "  Particionado vs Sequencial:  " << speedup_partitioned << "x";
                if (speedup_partitioned < 1.0) std::cout << " (PIOR)";
                else if (speedup_partitioned > 1.0) std::cout << " (MELHOR)";
                std::cout << std::endl;
                
                double speedup_part_vs_sched = parallel_result.execution_time / partitioned_result.execution_time;
                std::cout << "  Particionado vs Scheduler:   " << speedup_part_vs_sched << "x";
                if (speedup_part_vs_sched < 1.0) std::cout << " (PIOR)";
                else if (speedup_part_vs_sched > 1.0) std::cout << " (MELHOR)";
                std::cout << std::endl;
            }
            
            std::cout << "\n📈 THROUGHPUT (documentos/segundo):" << std::endl;
            std::cout << "  Scheduler:     " << (input_data.size() / parallel_result.execution_time) << std::endl;
            std::cout << "  Sequencial:    " << (input_data.size() / sequential_result.execution_time) << std::endl;
            std::cout << "  Particionado:  " << (input_data.size() / partitioned_result.execution_time) << std::endl;
            
            std::cout << "\n✅ RECOMENDAÇÃO:" << std::endl;
            if (partitioned_result.execution_time < sequential_result.execution_time && 
                partitioned_result.execution_time < parallel_result.execution_time) {
                std::cout << "  🏆 PARTICIONAMENTO é a melhor estratégia para este volume de dados!" << std::endl;
            } else if (sequential_result.execution_time < parallel_result.execution_time) {
                std::cout << "  🔄 SEQUENCIAL ainda é melhor (overhead de paralelização > benefício)" << std::endl;
            } else {
                std::cout << "  ⚡ PARALELO TRADICIONAL oferece melhor performance" << std::endl;
            }
        }

        return std::make_pair(parallel_result, sequential_result);
    }

    ComparisonResult PipelineManager::runFullComparison(const std::vector<std::string>& input_data) {
        ComparisonResult result;
        
        // Executa os três modos de pipeline
        std::cout << "\n🚀 INICIANDO COMPARAÇÃO COMPLETA DE PERFORMANCE 🚀" << std::endl;
        std::cout << "Testando " << input_data.size() << " documentos com " 
                  << config.num_workers << " workers disponíveis" << std::endl;
        
        result.parallel_result = runParallel(input_data);
        result.sequential_result = runSequential(input_data, true);
        result.partitioned_result = runParallelPartitioned(input_data);

        // Imprimir comparação detalhada
        if (result.parallel_result.success && result.sequential_result.success && result.partitioned_result.success) {
            std::cout << "\n📊 === ANÁLISE COMPARATIVA DE PERFORMANCE === 📊" << std::endl;
            std::cout << std::fixed << std::setprecision(4);
            
            std::cout << "\n⏱️  TEMPOS DE EXECUÇÃO:" << std::endl;
            std::cout << "  Pipeline Paralelo (Scheduler):     " << result.parallel_result.execution_time << " segundos" << std::endl;
            std::cout << "  Pipeline Sequencial (Thread Única): " << result.sequential_result.execution_time << " segundos" << std::endl;
            std::cout << "  Pipeline Paralelo (Particionado):   " << result.partitioned_result.execution_time << " segundos" << std::endl;
            
            std::cout << "\n🚀 SPEEDUPS:" << std::endl;
            if (result.sequential_result.execution_time > 0) {
                double speedup_scheduler = result.sequential_result.execution_time / result.parallel_result.execution_time;
                double speedup_partitioned = result.sequential_result.execution_time / result.partitioned_result.execution_time;
                
                std::cout << "  Scheduler vs Sequencial:     " << speedup_scheduler << "x";
                if (speedup_scheduler < 1.0) std::cout << " (PIOR)";
                else if (speedup_scheduler > 1.0) std::cout << " (MELHOR)";
                std::cout << std::endl;
                
                std::cout << "  Particionado vs Sequencial:  " << speedup_partitioned << "x";
                if (speedup_partitioned < 1.0) std::cout << " (PIOR)";
                else if (speedup_partitioned > 1.0) std::cout << " (MELHOR)";
                std::cout << std::endl;
                
                double speedup_part_vs_sched = result.parallel_result.execution_time / result.partitioned_result.execution_time;
                std::cout << "  Particionado vs Scheduler:   " << speedup_part_vs_sched << "x";
                if (speedup_part_vs_sched < 1.0) std::cout << " (PIOR)";
                else if (speedup_part_vs_sched > 1.0) std::cout << " (MELHOR)";
                std::cout << std::endl;
            }
            
            std::cout << "\n📈 THROUGHPUT (documentos/segundo):" << std::endl;
            std::cout << "  Scheduler:     " << (input_data.size() / result.parallel_result.execution_time) << std::endl;
            std::cout << "  Sequencial:    " << (input_data.size() / result.sequential_result.execution_time) << std::endl;
            std::cout << "  Particionado:  " << (input_data.size() / result.partitioned_result.execution_time) << std::endl;
            
            std::cout << "\n✅ RECOMENDAÇÃO:" << std::endl;
            if (result.partitioned_result.execution_time < result.sequential_result.execution_time && 
                result.partitioned_result.execution_time < result.parallel_result.execution_time) {
                std::cout << "  🏆 PARTICIONAMENTO é a melhor estratégia para este volume de dados!" << std::endl;
            } else if (result.sequential_result.execution_time < result.parallel_result.execution_time) {
                std::cout << "  🔄 SEQUENCIAL ainda é melhor (overhead de paralelização > benefício)" << std::endl;
            } else {
                std::cout << "  ⚡ PARALELO TRADICIONAL oferece melhor performance" << std::endl;
            }
        }

        return result;
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
        stats["parallel_time"] = last_parallel_time;
        stats["sequential_time"] = last_sequential_time;
        stats["partitioned_time"] = last_partitioned_time;
        
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
        last_parallel_time = 0.0;
        last_sequential_time = 0.0;
        last_partitioned_time = 0.0;
    }

    size_t PipelineManager::calculateOptimalChunkSize(size_t total_size, size_t num_workers) {
        // Calcula tamanho de chunk que maximize a utilização dos workers
        // Considera overhead de criação de threads vs. granularidade de trabalho
        
        if (total_size <= num_workers) {
            return 1; // Um documento por worker se há poucos documentos
        }
        
        // Chunk base: divide igualmente
        size_t base_chunk_size = total_size / num_workers;
        
        // Ajusta para ter granularidade mínima (evita chunks muito pequenos)
        const size_t MIN_CHUNK_SIZE = 50;
        const size_t MAX_CHUNK_SIZE = 1000;
        
        if (base_chunk_size < MIN_CHUNK_SIZE) {
            // Se chunks ficarem muito pequenos, prefere menos workers
            return std::min(static_cast<size_t>(total_size / (num_workers / 2)), MAX_CHUNK_SIZE);
        }
        
        return std::min(base_chunk_size, MAX_CHUNK_SIZE);
    }

    std::vector<std::vector<std::string>> PipelineManager::partitionData(
        const std::vector<std::string>& data, size_t chunk_size) {
        
        std::vector<std::vector<std::string>> chunks;
        
        for (size_t i = 0; i < data.size(); i += chunk_size) {
            size_t end = std::min(i + chunk_size, data.size());
            std::vector<std::string> chunk(data.begin() + i, data.begin() + end);
            chunks.push_back(std::move(chunk));
        }
        
        return chunks;
    }

    std::vector<std::string> PipelineManager::processChunkSequentially(
        const std::vector<std::string>& chunk_data, size_t chunk_id) {
        
        // Cria uma cópia local dos dados para processamento
        std::vector<std::string> processed_data = chunk_data;
        
        // Aplica todas as etapas do pipeline sequencialmente neste chunk
        // Isso garante que cada chunk passe pelo pipeline completo independentemente
        
        // Note: chunk_id é usado apenas para debug/logging se necessário
        (void)chunk_id; // Suprime warning de parâmetro não usado
        
        TextProcessor::cleanTextSequential(processed_data);
        TextProcessor::normalizeTextSequential(processed_data);
        TextProcessor::wordTokenizationSequential(processed_data);
        TextProcessor::bpeTokenization(processed_data);
        TextProcessor::partitionTokens(processed_data, config.max_sequence_length);
        TextProcessor::addSpecialTokens(processed_data);
        TextProcessor::tokensToIndices(processed_data);
        TextProcessor::generateEmbeddings(processed_data);
        
        return processed_data;
    }

    std::vector<std::string> PipelineManager::mergeProcessedChunks(
        const std::vector<std::vector<std::string>>& processed_chunks) {
        
        std::vector<std::string> merged_data;
        
        // Calcula o tamanho total para pré-alocar memória
        size_t total_size = 0;
        for (const auto& chunk : processed_chunks) {
            total_size += chunk.size();
        }
        merged_data.reserve(total_size);
        
        // Reconstrói os dados na ordem original
        for (const auto& chunk : processed_chunks) {
            merged_data.insert(merged_data.end(), chunk.begin(), chunk.end());
        }
        
        return merged_data;
    }

} // namespace pipeline
} // namespace legal_doc_pipeline
