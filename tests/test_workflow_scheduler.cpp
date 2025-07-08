#include <gtest/gtest.h>
#include "../include/scheduler/workflow_scheduler.h"
#include "../include/types.h"
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>

/**
 * @file test_workflow_scheduler.cpp
 * @brief Testes unitários para a classe WorkflowScheduler
 */

using namespace legal_doc_pipeline;
using namespace legal_doc_pipeline::scheduler;

class WorkflowSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        scheduler = std::make_unique<WorkflowScheduler>();
        test_data = {
            "Primeiro texto de teste",
            "Segundo texto para processamento",
            "Terceiro documento com mais conteúdo",
            "Último texto da lista de teste"
        };
        execution_counter = 0;
    }
    
    void TearDown() override {
        scheduler.reset();
    }
    
    std::unique_ptr<WorkflowScheduler> scheduler;
    std::vector<std::string> test_data;
    std::atomic<int> execution_counter;
    
    // Função helper para criar tarefas de teste
    std::function<void(std::vector<std::string>&)> createTestTask(int task_id) {
        return [this, task_id](std::vector<std::string>& texts) {
            execution_counter++;
            // Simula processamento
            for (auto& text : texts) {
                text += " [Processado por Task" + std::to_string(task_id) + "]";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };
    }
};

// Teste básico de adição de tarefa
TEST_F(WorkflowSchedulerTest, AddSingleTask) {
    Task task("TestTask", TaskType::TEXT_CLEANING, 10, createTestTask(1));
    
    EXPECT_NO_THROW(scheduler->addTask(task));
    
    auto stats = scheduler->getExecutionStats();
    EXPECT_EQ(stats["total_tasks"], 1);
}

// Teste de execução de tarefa única
TEST_F(WorkflowSchedulerTest, RunSingleTask) {
    Task task("TestTask", TaskType::TEXT_CLEANING, 10, createTestTask(1));
    scheduler->addTask(task);
    
    bool success = scheduler->run(test_data, 1);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(execution_counter.load(), 1);
    
    auto stats = scheduler->getExecutionStats();
    EXPECT_EQ(stats["completed_tasks"], 1);
    
    // Verifica se dados foram processados
    auto processed_data = scheduler->getProcessedData();
    EXPECT_EQ(processed_data.size(), test_data.size());
    for (const auto& text : processed_data) {
        EXPECT_TRUE(text.find("[Processado por Task1]") != std::string::npos);
    }
}

// Teste de múltiplas tarefas independentes
TEST_F(WorkflowSchedulerTest, RunMultipleIndependentTasks) {
    scheduler->addTask(Task("Task1", TaskType::TEXT_CLEANING, 10, createTestTask(1)));
    scheduler->addTask(Task("Task2", TaskType::NORMALIZATION, 20, createTestTask(2)));
    scheduler->addTask(Task("Task3", TaskType::WORD_TOKENIZATION, 30, createTestTask(3)));
    
    bool success = scheduler->run(test_data, 2);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(execution_counter.load(), 3);
    
    auto stats = scheduler->getExecutionStats();
    EXPECT_EQ(stats["total_tasks"], 3);
    EXPECT_EQ(stats["completed_tasks"], 3);
}

// Teste de dependências entre tarefas
TEST_F(WorkflowSchedulerTest, RunTasksWithDependencies) {
    scheduler->addTask(Task("TaskA", TaskType::TEXT_CLEANING, 10, createTestTask(1)));
    scheduler->addTask(Task("TaskB", TaskType::NORMALIZATION, 20, createTestTask(2)));
    scheduler->addTask(Task("TaskC", TaskType::WORD_TOKENIZATION, 30, createTestTask(3)));
    
    // Criar dependências: A -> B -> C
    scheduler->addDependency("TaskB", "TaskA");
    scheduler->addDependency("TaskC", "TaskB");
    
    bool success = scheduler->run(test_data, 2);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(execution_counter.load(), 3);
    
    // Verifica se as tarefas foram executadas na ordem correta
    auto processed_data = scheduler->getProcessedData();
    for (const auto& text : processed_data) {
        // Deve conter marcadores de todas as tarefas
        EXPECT_TRUE(text.find("[Processado por Task1]") != std::string::npos);
        EXPECT_TRUE(text.find("[Processado por Task2]") != std::string::npos);
        EXPECT_TRUE(text.find("[Processado por Task3]") != std::string::npos);
    }
}

// Teste de detecção de dependências circulares
TEST_F(WorkflowSchedulerTest, DetectCircularDependency) {
    scheduler->addTask(Task("TaskA", TaskType::TEXT_CLEANING, 10, createTestTask(1)));
    scheduler->addTask(Task("TaskB", TaskType::NORMALIZATION, 20, createTestTask(2)));
    scheduler->addTask(Task("TaskC", TaskType::WORD_TOKENIZATION, 30, createTestTask(3)));
    
    // Criar dependência circular: A -> B -> C -> A
    scheduler->addDependency("TaskB", "TaskA");
    scheduler->addDependency("TaskC", "TaskB");
    scheduler->addDependency("TaskA", "TaskC");
    
    bool success = scheduler->run(test_data, 1);
    
    // Deve falhar devido à dependência circular
    EXPECT_FALSE(success);
}

// Teste de prioridades
TEST_F(WorkflowSchedulerTest, TaskPriorities) {
    // Adicionar tarefas com prioridades diferentes (menor número = maior prioridade)
    scheduler->addTask(Task("LowPriority", TaskType::TEXT_CLEANING, 30, createTestTask(3)));
    scheduler->addTask(Task("HighPriority", TaskType::NORMALIZATION, 10, createTestTask(1)));
    scheduler->addTask(Task("MediumPriority", TaskType::WORD_TOKENIZATION, 20, createTestTask(2)));
    
    bool success = scheduler->run(test_data, 1);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(execution_counter.load(), 3);
}

// Teste de limpeza e reinicialização
TEST_F(WorkflowSchedulerTest, ClearAndReset) {
    scheduler->addTask(Task("Task1", TaskType::TEXT_CLEANING, 10, createTestTask(1)));
    scheduler->addTask(Task("Task2", TaskType::NORMALIZATION, 20, createTestTask(2)));
    
    auto stats_before = scheduler->getExecutionStats();
    EXPECT_EQ(stats_before["total_tasks"], 2);
    
    scheduler->clear();
    
    auto stats_after = scheduler->getExecutionStats();
    EXPECT_EQ(stats_after["total_tasks"], 0);
    EXPECT_EQ(stats_after["completed_tasks"], 0);
}

// Teste com diferentes números de workers
TEST_F(WorkflowSchedulerTest, DifferentWorkerCounts) {
    // Adicionar tarefas independentes
    for (int i = 1; i <= 5; ++i) {
        scheduler->addTask(Task("Task" + std::to_string(i), 
                               TaskType::TEXT_CLEANING, 
                               i * 10, 
                               createTestTask(i)));
    }
    
    // Teste com 1 worker
    execution_counter = 0;
    bool success1 = scheduler->run(test_data, 1);
    int count1 = execution_counter.load();
    
    scheduler->clear();
    for (int i = 1; i <= 5; ++i) {
        scheduler->addTask(Task("Task" + std::to_string(i), 
                               TaskType::TEXT_CLEANING, 
                               i * 10, 
                               createTestTask(i)));
    }
    
    // Teste com 3 workers
    execution_counter = 0;
    bool success3 = scheduler->run(test_data, 3);
    int count3 = execution_counter.load();
    
    EXPECT_TRUE(success1);
    EXPECT_TRUE(success3);
    EXPECT_EQ(count1, 5);
    EXPECT_EQ(count3, 5);
}

// Teste de thread safety
TEST_F(WorkflowSchedulerTest, ThreadSafety) {
    // Adicionar muitas tarefas independentes
    for (int i = 1; i <= 20; ++i) {
        scheduler->addTask(Task("Task" + std::to_string(i), 
                               TaskType::TEXT_CLEANING, 
                               i, 
                               createTestTask(i)));
    }
    
    bool success = scheduler->run(test_data, 4);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(execution_counter.load(), 20);
    
    auto stats = scheduler->getExecutionStats();
    EXPECT_EQ(stats["total_tasks"], 20);
    EXPECT_EQ(stats["completed_tasks"], 20);
}

// Teste de dados vazios
TEST_F(WorkflowSchedulerTest, EmptyData) {
    std::vector<std::string> empty_data;
    
    scheduler->addTask(Task("Task1", TaskType::TEXT_CLEANING, 10, createTestTask(1)));
    
    bool success = scheduler->run(empty_data, 1);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(execution_counter.load(), 1);
    
    auto processed_data = scheduler->getProcessedData();
    EXPECT_TRUE(processed_data.empty());
}

// Teste de estado do scheduler
TEST_F(WorkflowSchedulerTest, SchedulerState) {
    scheduler->addTask(Task("Task1", TaskType::TEXT_CLEANING, 10, createTestTask(1)));
    
    // Estado inicial
    EXPECT_FALSE(scheduler->isRunning());
    
    // Durante execução (teste complexo - pode precisar de threading)
    bool success = scheduler->run(test_data, 1);
    
    // Estado final
    EXPECT_TRUE(success);
    EXPECT_FALSE(scheduler->isRunning());
    EXPECT_TRUE(scheduler->allTasksCompleted());
}

// Teste de dependência inexistente
TEST_F(WorkflowSchedulerTest, NonExistentDependency) {
    scheduler->addTask(Task("TaskA", TaskType::TEXT_CLEANING, 10, createTestTask(1)));
    
    // Tentar adicionar dependência para tarefa inexistente deve retornar false
    bool result = scheduler->addDependency("TaskA", "NonExistentTask");
    EXPECT_FALSE(result);
    
    // Execução deve falhar devido à dependência inválida
    bool success = scheduler->run(test_data, 1);
    EXPECT_FALSE(success);
}
