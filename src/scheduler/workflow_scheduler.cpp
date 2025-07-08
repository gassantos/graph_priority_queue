#include "../../include/scheduler/workflow_scheduler.h"
#include <iostream>
#include <algorithm>
#include <set>

namespace legal_doc_pipeline {
namespace scheduler {

    WorkflowScheduler::WorkflowScheduler() 
        : completed_task_count(0), shutdown_requested(false), has_dependency_errors(false) {}

    WorkflowScheduler::~WorkflowScheduler() {
        shutdown();
    }

    void WorkflowScheduler::addTask(const Task& task) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace(task.id, task);
    }

    bool WorkflowScheduler::addDependency(const std::string& task_id, const std::string& dependency_id) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        
        // Garante que as tarefas existem
        if (tasks.find(task_id) == tasks.end() || tasks.find(dependency_id) == tasks.end()) {
            std::cerr << "Erro: Tarefa '" << task_id << "' ou '" << dependency_id 
                      << "' não encontrada ao adicionar dependência." << std::endl;
            has_dependency_errors = true;
            return false;
        }
        
        tasks.at(task_id).dependencies.push_back(dependency_id);
        tasks.at(dependency_id).dependents.push_back(task_id);
        tasks.at(task_id).remaining_dependencies++;
        return true;
    }

    bool WorkflowScheduler::run(const std::vector<std::string>& input_data, int num_workers) {
        // Verifica se há erros de dependência
        if (has_dependency_errors) {
            std::cerr << "Erro: Há dependências inválidas no grafo!" << std::endl;
            return false;
        }
        
        // Valida o grafo antes de executar
        if (!validateDependencyGraph()) {
            std::cerr << "Erro: Grafo de dependências contém ciclos!" << std::endl;
            return false;
        }

        processed_texts = input_data;
        completed_task_count = 0;
        shutdown_requested = false;

        // Inicia os workers
        workers.clear();
        workers.reserve(num_workers);
        for (int i = 0; i < num_workers; ++i) {
            workers.emplace_back(&WorkflowScheduler::workerThread, this);
        }

        // Inicializa a fila de tarefas prontas
        initializeReadyQueue();

        // Aguarda todos os workers terminarem
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        workers.clear();
        std::cout << "Todos os workers terminaram a execução." << std::endl;
        return allTasksCompleted();
    }

    void WorkflowScheduler::shutdown() {
        shutdown_requested = true;
        cv_tasks_ready.notify_all();
        
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers.clear();
    }

    void WorkflowScheduler::workerThread() {
        while (!shutdown_requested) {
            Task* current_task_ptr = nullptr;
            bool task_found = false;

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv_tasks_ready.wait(lock, [this]{
                    return !ready_queue.empty() || 
                           allTasksCompleted() || 
                           shutdown_requested;
                });

                if (shutdown_requested || (allTasksCompleted() && ready_queue.empty())) {
                    std::cout << "Worker encerrando: todas as tarefas concluídas. (ID thread: " 
                              << std::this_thread::get_id() << ")" << std::endl;
                    break;
                }

                if (!ready_queue.empty()) {
                    current_task_ptr = ready_queue.top();
                    ready_queue.pop();
                    task_found = true;
                    std::cout << "Worker (ID: " << std::this_thread::get_id()
                              << ") pegou a tarefa: " << current_task_ptr->id << std::endl;
                }
            }

            if (task_found && current_task_ptr) {
                try {
                    current_task_ptr->operation(processed_texts);
                    markTaskCompleted(current_task_ptr->id);
                } catch (const std::exception& e) {
                    std::cerr << "Erro ao executar tarefa " << current_task_ptr->id 
                              << ": " << e.what() << std::endl;
                    shutdown_requested = true;
                    cv_tasks_ready.notify_all();
                    break;
                }
            }
        }
    }

    void WorkflowScheduler::markTaskCompleted(const std::string& task_id) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        
        Task& completed_task = tasks.at(task_id);
        completed_task.is_completed = true;
        completed_task_count++;
        
        std::cout << "Tarefa '" << task_id << "' finalizada! Total concluídas: " 
                  << completed_task_count.load() << std::endl;

        // Atualiza dependências das tarefas sucessoras
        for (const std::string& dependent_id : completed_task.dependents) {
            Task& dependent_task = tasks.at(dependent_id);
            dependent_task.remaining_dependencies--;
            
            if (dependent_task.remaining_dependencies == 0 && !dependent_task.is_completed) {
                ready_queue.push(&dependent_task);
                std::cout << "Tarefa '" << dependent_id << "' está pronta e adicionada à fila." << std::endl;
            }
        }
        
        cv_tasks_ready.notify_all();
    }

    void WorkflowScheduler::initializeReadyQueue() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        
        for (auto& pair : tasks) {
            if (pair.second.remaining_dependencies == 0) {
                ready_queue.push(&pair.second);
                std::cout << "Tarefa inicial '" << pair.second.id 
                          << "' adicionada à fila de prontos." << std::endl;
            }
        }
        
        cv_tasks_ready.notify_all();
    }

    bool WorkflowScheduler::allTasksCompleted() const {
        return completed_task_count.load() == tasks.size();
    }

    const std::vector<std::string>& WorkflowScheduler::getProcessedData() const {
        return processed_texts;
    }

    std::map<std::string, size_t> WorkflowScheduler::getExecutionStats() const {
        std::map<std::string, size_t> stats;
        stats["total_tasks"] = tasks.size();
        stats["completed_tasks"] = completed_task_count.load();
        stats["pending_tasks"] = tasks.size() - completed_task_count.load();
        stats["workers_count"] = workers.size();
        
        return stats;
    }

    bool WorkflowScheduler::isRunning() const {
        return !workers.empty() && !shutdown_requested;
    }

    void WorkflowScheduler::clear() {
        shutdown();
        
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.clear();
        
        // Limpa a fila de prontos
        while (!ready_queue.empty()) {
            ready_queue.pop();
        }
        
        processed_texts.clear();
        completed_task_count = 0;
        shutdown_requested = false;
        has_dependency_errors = false;
    }

    bool WorkflowScheduler::validateDependencyGraph() const {
        // Implementa algoritmo de detecção de ciclos usando DFS
        std::set<std::string> visited;
        std::set<std::string> rec_stack;
        
        std::function<bool(const std::string&)> has_cycle = [&](const std::string& task_id) -> bool {
            visited.insert(task_id);
            rec_stack.insert(task_id);
            
            auto it = tasks.find(task_id);
            if (it != tasks.end()) {
                for (const std::string& dependent : it->second.dependents) {
                    if (rec_stack.count(dependent) || 
                        (!visited.count(dependent) && has_cycle(dependent))) {
                        return true;
                    }
                }
            }
            
            rec_stack.erase(task_id);
            return false;
        };
        
        for (const auto& pair : tasks) {
            if (!visited.count(pair.first)) {
                if (has_cycle(pair.first)) {
                    return false;
                }
            }
        }
        
        return true;
    }

    std::string WorkflowScheduler::getDependencyGraphString() const {
        std::string result = "Grafo de Dependências:\n";
        
        for (const auto& pair : tasks) {
            const Task& task = pair.second;
            result += "Tarefa: " + task.id + " (Prioridade: " + std::to_string(task.priority) + ")\n";
            
            if (!task.dependencies.empty()) {
                result += "  Dependências: ";
                for (size_t i = 0; i < task.dependencies.size(); ++i) {
                    result += task.dependencies[i];
                    if (i < task.dependencies.size() - 1) result += ", ";
                }
                result += "\n";
            }
            
            if (!task.dependents.empty()) {
                result += "  Sucessores: ";
                for (size_t i = 0; i < task.dependents.size(); ++i) {
                    result += task.dependents[i];
                    if (i < task.dependents.size() - 1) result += ", ";
                }
                result += "\n";
            }
            result += "\n";
        }
        
        return result;
    }

} // namespace scheduler
} // namespace legal_doc_pipeline
