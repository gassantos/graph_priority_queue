#ifndef SCHEDULER_WORKFLOW_SCHEDULER_H
#define SCHEDULER_WORKFLOW_SCHEDULER_H

#include "../types.h"
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <vector>

/**
 * @file workflow_scheduler.h
 * @brief Scheduler de workflow baseado em grafo de dependências
 * 
 * Implementa um scheduler que executa tarefas respeitando suas dependências
 * e prioridades, usando um pool de threads trabalhadoras.
 */

namespace legal_doc_pipeline {
namespace scheduler {

    /**
     * @brief Scheduler de workflow com execução paralela baseada em grafo
     */
    class WorkflowScheduler {
    private:
        std::map<std::string, Task> tasks;                              ///< Mapa de todas as tarefas por ID
        std::priority_queue<Task*, std::vector<Task*>, TaskPtrCompare> ready_queue; ///< Fila de tarefas prontas
        std::mutex queue_mutex;                                         ///< Mutex para proteger acesso às estruturas
        std::condition_variable cv_tasks_ready;                         ///< Condição para sinalizar tarefas prontas
        std::atomic<int> completed_task_count;                          ///< Contador de tarefas concluídas
        std::vector<std::string> processed_texts;                       ///< Dados sendo processados
        std::vector<std::thread> workers;                               ///< Pool de threads trabalhadoras
        std::atomic<bool> shutdown_requested;                           ///< Flag para shutdown gracioso

        /**
         * @brief Função executada por cada thread trabalhadora
         */
        void workerThread();

        /**
         * @brief Marca uma tarefa como concluída e atualiza dependências
         * @param task_id ID da tarefa concluída
         */
        void markTaskCompleted(const std::string& task_id);

        /**
         * @brief Inicializa a fila de tarefas prontas
         */
        void initializeReadyQueue();

        /**
         * @brief Verifica se todas as tarefas foram concluídas
         * @return true se todas as tarefas foram concluídas
         */
        bool allTasksCompleted() const;

    public:
        /**
         * @brief Construtor
         */
        WorkflowScheduler();

        /**
         * @brief Destrutor
         */
        ~WorkflowScheduler();

        /**
         * @brief Adiciona uma tarefa ao scheduler
         * @param task Tarefa a ser adicionada
         */
        void addTask(const Task& task);

        /**
         * @brief Adiciona uma dependência entre tarefas
         * @param task_id ID da tarefa dependente
         * @param dependency_id ID da tarefa da qual depende
         */
        void addDependency(const std::string& task_id, const std::string& dependency_id);

        /**
         * @brief Executa o workflow com o número especificado de workers
         * @param input_data Dados de entrada para processamento
         * @param num_workers Número de threads trabalhadoras
         * @return true se a execução foi bem-sucedida
         */
        bool run(const std::vector<std::string>& input_data, int num_workers = 4);

        /**
         * @brief Para a execução do scheduler graciosamente
         */
        void shutdown();

        /**
         * @brief Obtém os dados processados
         * @return Referência para os dados processados
         */
        const std::vector<std::string>& getProcessedData() const;

        /**
         * @brief Obtém estatísticas de execução
         * @return Mapa com estatísticas
         */
        std::map<std::string, size_t> getExecutionStats() const;

        /**
         * @brief Verifica se o scheduler está rodando
         * @return true se o scheduler está ativo
         */
        bool isRunning() const;

        /**
         * @brief Limpa todas as tarefas e reinicia o estado
         */
        void clear();

        /**
         * @brief Valida a consistência do grafo de dependências
         * @return true se o grafo é válido (sem ciclos)
         */
        bool validateDependencyGraph() const;

        /**
         * @brief Obtém uma representação em string do grafo de dependências
         * @return String com a representação do grafo
         */
        std::string getDependencyGraphString() const;

        // Desabilita cópia e atribuição
        WorkflowScheduler(const WorkflowScheduler&) = delete;
        WorkflowScheduler& operator=(const WorkflowScheduler&) = delete;
    };

} // namespace scheduler
} // namespace legal_doc_pipeline

#endif // SCHEDULER_WORKFLOW_SCHEDULER_H
