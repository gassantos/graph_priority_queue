#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <functional>
#include <atomic>

/**
 * @file types.h
 * @brief Definições de tipos fundamentais utilizados em todo o pipeline
 * 
 * Este arquivo contém as definições de tipos, enums e estruturas básicas
 * que são compartilhadas entre diferentes módulos do sistema.
 */

namespace legal_doc_pipeline {

    /**
     * @brief Enumeração dos tipos de tarefas disponíveis no pipeline
     */
    enum class TaskType {
        TEXT_CLEANING,
        NORMALIZATION,
        WORD_TOKENIZATION,
        BPE_TOKENIZATION,
        PARTITION_TOKENS,
        ADD_SPECIAL_TOKENS,
        TOKENS_TO_INDICES,
        GENERATE_EMBEDDINGS
    };

    /**
     * @brief Estrutura que representa uma tarefa no grafo de dependências
     */
    struct Task {
        std::string id;                                                    ///< Identificador único da tarefa
        TaskType type;                                                     ///< Tipo da tarefa
        int priority;                                                      ///< Prioridade (menor valor = maior prioridade)
        std::vector<std::string> dependencies;                           ///< IDs das tarefas predecessoras
        std::vector<std::string> dependents;                             ///< IDs das tarefas sucessoras
        std::function<void(std::vector<std::string>&)> operation;        ///< Função da tarefa
        std::atomic<int> remaining_dependencies;                         ///< Contador de dependências não satisfeitas
        bool is_completed;                                               ///< Flag de conclusão

        /**
         * @brief Construtor da tarefa
         * @param id Identificador único
         * @param type Tipo da tarefa
         * @param priority Prioridade da tarefa
         * @param op Função a ser executada
         */
        Task(std::string id, TaskType type, int priority, std::function<void(std::vector<std::string>&)> op);

        /**
         * @brief Construtor de cópia
         * @param other Tarefa a ser copiada
         */
        Task(const Task& other);

        /**
         * @brief Operador de comparação para ordenação por prioridade
         * @param other Tarefa a ser comparada
         * @return true se esta tarefa tem menor prioridade que other
         */
        bool operator<(const Task& other) const;
    };

    /**
     * @brief Comparador para ponteiros de Task na fila de prioridade
     */
    struct TaskPtrCompare {
        bool operator()(const Task* a, const Task* b) const;
    };

    /**
     * @brief Configuração para execução do pipeline
     */
    struct PipelineConfig {
        int num_workers = 4;                    ///< Número de threads trabalhadoras
        bool enable_debug = false;              ///< Habilita output de debug
        size_t max_sequence_length = 128;       ///< Tamanho máximo de sequência
        std::string vocab_file = "vocab.txt";   ///< Arquivo de vocabulário
        std::string merges_file = "merges.txt"; ///< Arquivo de merges BPE
    };

    /**
     * @brief Resultado da execução do pipeline
     */
    struct PipelineResult {
        std::vector<std::string> processed_data;  ///< Dados processados
        double execution_time;                    ///< Tempo de execução em segundos
        size_t tasks_completed;                   ///< Número de tarefas completadas
        bool success;                             ///< Flag de sucesso
        std::string error_message;                ///< Mensagem de erro, se houver
    };

} // namespace legal_doc_pipeline

#endif // TYPES_H
