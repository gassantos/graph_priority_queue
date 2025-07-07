#include "../include/types.h"

namespace legal_doc_pipeline {

    Task::Task(std::string id, TaskType type, int priority, std::function<void(std::vector<std::string>&)> op)
        : id(std::move(id)), type(type), priority(priority), operation(std::move(op)),
          remaining_dependencies(0), is_completed(false) {}

    Task::Task(const Task& other)
        : id(other.id), type(other.type), priority(other.priority), dependencies(other.dependencies),
          dependents(other.dependents), operation(other.operation),
          remaining_dependencies(other.remaining_dependencies.load()),
          is_completed(other.is_completed) {}

    bool Task::operator<(const Task& other) const {
        return priority > other.priority; // Min-heap por padrão, queremos Max-heap para prioridade
    }

    bool TaskPtrCompare::operator()(const Task* a, const Task* b) const {
        // Retorna true se 'a' deve vir DEPOIS de 'b' na fila (menor prioridade)
        // std::priority_queue é um max-heap, então para ter o menor 'priority' no topo,
        // precisamos que o operador '<' retorne true quando 'a' tem *maior* prioridade (menor valor numérico)
        return a->priority > b->priority;
    }

} // namespace legal_doc_pipeline
