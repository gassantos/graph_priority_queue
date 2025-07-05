#include <iostream>
#include <vector>
#include <string>
#include <functional>   // Para std::function
#include <thread>       // Para std::thread
#include <chrono>       // Para std::chrono
#include <algorithm>    // Para std::transform, std::min
#include <regex>        // Para std::regex
#include <map>          // Para std::map
#include <queue>        // Para std::priority_queue
#include <mutex>        // Para std::mutex
#include <condition_variable> // Para std::condition_variable
#include <atomic>       // Para std::atomic
#include <fstream>      // Para std::ifstream
#include <sstream>      // Para std::stringstream
#include <locale>       // Para std::tolower

// --- 1. Definição da Estrutura Task ---

enum class TaskType {
    TEXT_CLEANING,
    NORMALIZATION,
    WORD_TOKENIZATION,
    BPE_TOKENIZATION,
    PARTITION_TOKENS,
    ADD_SPECIAL_TOKENS,
    TOKENS_TO_INDICES,
    GENERATE_EMBEDDINGS,
    // Adicione outros tipos de tarefa conforme necessário
};

struct Task {
    std::string id;             // Identificador único da tarefa
    TaskType type;              // Tipo da tarefa (para dispatch)
    int priority;               // Prioridade da tarefa (menor valor = maior prioridade)
    std::vector<std::string> dependencies; // IDs das tarefas predecessoras
    std::vector<std::string> dependents;   // IDs das tarefas sucessoras
    std::function<void(std::vector<std::string>&)> operation; // Função a ser executada pela tarefa
    std::atomic<int> remaining_dependencies; // Contador de dependências não satisfeitas
    bool is_completed;          // Indica se a tarefa foi concluída

    Task(std::string id, TaskType type, int priority, std::function<void(std::vector<std::string>&)> op)
        : id(std::move(id)), type(type), priority(priority), operation(std::move(op)),
          remaining_dependencies(0), is_completed(false) {}

    // Construtor de cópia: essencial para que Task possa ser armazenado em std::map.
    // std::atomic não pode ser copiado diretamente, então precisamos carregar o valor.
    Task(const Task& other)
        : id(other.id), type(other.type), priority(other.priority), dependencies(other.dependencies),
          dependents(other.dependents), operation(other.operation),
          remaining_dependencies(other.remaining_dependencies.load()), // Copia o valor atômico
          is_completed(other.is_completed) {}

    // Como estamos usando ponteiros na priority_queue, os operadores < ou = para Task não serão usados lá.
    // No entanto, para comparação interna ou caso o operador seja chamado em outro lugar, é bom ter.
    // Sobrecarga do operador < para a fila de prioridade (menor priority = maior prioridade)
    // Este operador seria usado se Task estivesse diretamente na priority_queue.
    // Mas agora é usado pela TaskPtrCompare.
    bool operator<(const Task& other) const {
        return priority > other.priority; // Min-heap por padrão, queremos Max-heap para prioridade
    }
};

// --- Comparador para a fila de prioridade de ponteiros de Task ---
struct TaskPtrCompare {
    bool operator()(const Task* a, const Task* b) const {
        // Retorna true se 'a' deve vir DEPOIS de 'b' na fila (menor prioridade)
        // std::priority_queue é um max-heap, então para ter o menor 'priority' no topo,
        // precisamos que o operador '<' retorne true quando 'a' tem *maior* prioridade (menor valor numérico)
        return a->priority > b->priority;
    }
};


// --- 2. Funções de Pré-processamento de Dados ---

// Função para ler o CSV
std::vector<std::string> readCsvColumn(const std::string& filename, const std::string& column_name) {
    std::vector<std::string> column_data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo CSV: " << filename << std::endl;
        return column_data;
    }

    std::string line;
    std::getline(file, line); // Lê o cabeçalho

    std::stringstream ss_header(line);
    std::string header_item;
    std::vector<std::string> headers;
    int column_index = -1;
    int current_column = 0;

    // Parse do cabeçalho para encontrar o índice da coluna
    while (std::getline(ss_header, header_item, ';')) { // Assumindo delimitador ';'
        // Remover aspas extras que podem vir com o nome da coluna no CSV
        if (header_item.length() > 0 && header_item[0] == '"' && header_item.back() == '"') {
            header_item = header_item.substr(1, header_item.length() - 2);
        }

        headers.push_back(header_item);
        if (header_item == column_name) {
            column_index = current_column;
        }
        current_column++;
    }

    if (column_index == -1) {
        std::cerr << "Coluna '" << column_name << "' não encontrada no CSV." << std::endl;
        return column_data;
    }

    while (std::getline(file, line)) {
        std::stringstream ss_line(line);
        std::string cell;
        int current_cell = 0;
        bool found_column = false;
        // Percorre as células para encontrar a da coluna desejada
        while (std::getline(ss_line, cell, ';')) { // Assumindo delimitador ';'
            if (current_cell == column_index) {
                // Remover aspas extras se a célula estiver entre aspas
                if (cell.length() > 0 && cell[0] == '"' && cell.back() == '"') {
                    cell = cell.substr(1, cell.length() - 2);
                }
                column_data.push_back(cell);
                found_column = true;
                break;
            }
            current_cell++;
        }
        // Se a linha não tem colunas suficientes para a coluna_index, adiciona uma string vazia
        if (!found_column) {
            column_data.push_back("");
        }
    }
    file.close();
    return column_data;
}

// 1. Limpeza do texto (Remover HTML, espaços extras, caracteres desnecessários)
void cleanText(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando CleanText..." << std::endl;
    for (std::string& text : texts) {
        text = std::regex_replace(text, std::regex("<.*?>"), " "); // Substitui HTML por espaço
        text = std::regex_replace(text, std::regex("[^a-zA-Z0-9\\sÀ-ÿ]", std::regex::ECMAScript | std::regex::collate), " "); // Mantém alfanuméricos, acentuados e espaços
        text = std::regex_replace(text, std::regex("\\s+"), " "); // Múltiplos espaços para um
        text = std::regex_replace(text, std::regex("^\\s+|\\s+$"), ""); // Espaços no início/fim
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    std::cout << "  [Task] CleanText concluído." << std::endl;
}

// 2. Normalização (Padronização)
void normalizeText(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando NormalizeText..." << std::endl;
    for (std::string& text : texts) {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char c){ return std::tolower(c); });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    std::cout << "  [Task] NormalizeText concluído." << std::endl;
}

// 3. Tokenização de palavras (simplificada, apenas divide por espaço)
void wordTokenization(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando WordTokenization (simulado)..." << std::endl;
    // Em uma implementação real, cada string seria tokenizada em um vetor de palavras.
    // Para manter a assinatura e o fluxo de dados no pipeline (vector<string>),
    // esta função apenas simula a operação.
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    std::cout << "  [Task] WordTokenization concluído." << std::endl;
}

// 4. Tokenizador BPE (Byte Pair Encoding) - Simulado
void bpeTokenization(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando BPETokenization (simulado)..." << std::endl;
    // Implementação real exigiria uma biblioteca BPE.
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    std::cout << "  [Task] BPETokenization concluído." << std::endl;
}

// 5. Particionar texto em tokens - Simulado
void partitionTokens(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando PartitionTokens (simulado)..." << std::endl;
    // Em um cenário real, dividiria as sequências de tokens em chunks.
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    std::cout << "  [Task] PartitionTokens concluído." << std::endl;
}

// 6. Adicionar tokens especiais (CLS, SEP, EOF) - Simulado
void addSpecialTokens(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando AddSpecialTokens..." << std::endl;
    for (std::string& text : texts) {
        text = "[CLS] " + text + " [SEP] [EOF]";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    std::cout << "  [Task] AddSpecialTokens concluído." << std::endl;
}

// 7. Conversão de tokens para índices - Simulado
void tokensToIndices(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando TokensToIndices (simulado)..." << std::endl;
    // Em uma implementação real, usaria um vocabulário para mapear tokens para IDs.
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    std::cout << "  [Task] TokensToIndices concluído." << std::endl;
}

// 8. Geração de vetores de embeddings - Simulado
void generateEmbeddings(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando GenerateEmbeddings (simulado)..." << std::endl;
    // Esta etapa real envolve um modelo de embedding e geraria vetores.
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    std::cout << "  [Task] GenerateEmbeddings concluído." << std::endl;
}


// --- 3. Implementação do WorkflowScheduler ---

class WorkflowScheduler {
public:
    std::map<std::string, Task> tasks; // Mapa de todas as tarefas por ID
    // ready_queue agora armazena ponteiros para Task e usa o comparador personalizado
    std::priority_queue<Task*, std::vector<Task*>, TaskPtrCompare> ready_queue;
    std::mutex queue_mutex; // Mutex para proteger o acesso à ready_queue e tasks
    std::condition_variable cv_tasks_ready; // Variável de condição para sinalizar tarefas prontas
    std::atomic<int> completed_task_count; // Contador atômico de tarefas concluídas

    std::vector<std::string> processed_texts; // Variável para armazenar o texto pré-processado

    WorkflowScheduler() : completed_task_count(0) {}

    void addTask(const Task& task) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace(task.id, task);
    }

    void addDependency(const std::string& task_id, const std::string& dependency_id) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        // Garante que as tarefas existem antes de adicionar dependência
        if (tasks.find(task_id) == tasks.end() || tasks.find(dependency_id) == tasks.end()) {
            std::cerr << "Erro: Tarefa '" << task_id << "' ou '" << dependency_id << "' não encontrada ao adicionar dependência." << std::endl;
            return;
        }
        tasks.at(task_id).dependencies.push_back(dependency_id);
        tasks.at(dependency_id).dependents.push_back(task_id);
        tasks.at(task_id).remaining_dependencies++; // Incrementa o contador de dependências
    }

    void initializeReadyQueue() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        for (auto& pair : tasks) {
            // Se a tarefa não tem dependências, ela está pronta para ser executada
            if (pair.second.remaining_dependencies == 0) {
                ready_queue.push(&pair.second); // Adiciona o ponteiro da tarefa
                std::cout << "Tarefa inicial '" << pair.second.id << "' adicionada à fila de prontos." << std::endl;
            }
        }
        cv_tasks_ready.notify_all(); // Notifica que tarefas iniciais estão prontas
    }

    void markTaskCompleted(const std::string& task_id) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        Task& completed_task = tasks.at(task_id);
        completed_task.is_completed = true;
        completed_task_count++; // Incrementa o contador global de tarefas concluídas
        std::cout << "Tarefa '" << task_id << "' concluída. Total concluídas: " << completed_task_count << std::endl;

        // Para cada tarefa que depende desta tarefa concluída
        for (const std::string& dependent_id : completed_task.dependents) {
            Task& dependent_task = tasks.at(dependent_id);
            dependent_task.remaining_dependencies--; // Decrementa a contagem de dependências
            if (dependent_task.remaining_dependencies == 0 && !dependent_task.is_completed) {
                ready_queue.push(&dependent_task); // Adiciona o ponteiro da tarefa à fila de prontos
                std::cout << "Tarefa '" << dependent_id << "' está pronta e adicionada à fila." << std::endl;
            }
        }
        cv_tasks_ready.notify_all(); // Notifica threads que aguardam tarefas
    }

    // Função para simular o worker que executa as tarefas
    void worker_thread() {
        while (true) {
            Task* current_task_ptr = nullptr; // Ponteiro para a tarefa atual
            bool task_found = false;

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv_tasks_ready.wait(lock, [this]{
                    // Condição de saída para workers: todas as tarefas foram concluídas E a fila está vazia.
                    // Ou se há tarefas na fila.
                    return !ready_queue.empty() || (completed_task_count.load() == tasks.size());
                });

                // Verifica novamente a condição de saída após ser acordado
                if (completed_task_count.load() == tasks.size() && ready_queue.empty()) {
                    std::cout << "Worker encerrando: todas as tarefas concluídas. (ID thread: " << std::this_thread::get_id() << ")" << std::endl;
                    break; // Sai do loop do worker
                }

                if (!ready_queue.empty()) {
                    current_task_ptr = ready_queue.top(); // Pega o ponteiro
                    ready_queue.pop(); // Remove o ponteiro da fila
                    task_found = true;
                    std::cout << "Worker (ID: " << std::this_thread::get_id() << ") pegou a tarefa: " << current_task_ptr->id << std::endl;
                }
            } // Lock é liberado aqui

            if (task_found && current_task_ptr) {
                // Executa a operação da tarefa através do ponteiro
                current_task_ptr->operation(processed_texts); // Passa o vetor de textos para a função da tarefa
                markTaskCompleted(current_task_ptr->id);
            }
        }
    }

    void run(int num_workers) {
        processed_texts.clear(); // Limpa o vetor de textos antes de iniciar, se houver execuções anteriores
        completed_task_count = 0; // Reseta o contador de tarefas concluídas

        // Inicia os workers
        std::vector<std::thread> workers;
        for (int i = 0; i < num_workers; ++i) {
            workers.emplace_back(&WorkflowScheduler::worker_thread, this);
        }

        // Inicializa a fila de prontos com tarefas sem dependências
        initializeReadyQueue();

        // Aguarda todos os workers terminarem
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        std::cout << "Todos os workers terminaram a execução." << std::endl;
    }
};

// --- 4. Função Principal (main) ---

int main() {
    std::cout << "Iniciando pipeline de pré-processamento de dados jurídicos." << std::endl;

    // 1. Carregar os dados
    std::string csv_filename = "Sumarizacao_Doc_TCERJ.csv";
    std::string column_to_process = "Texto";
    std::cout << "Lendo coluna '" << column_to_process << "' do arquivo '" << csv_filename << "'..." << std::endl;

    // Criar uma instância do scheduler
    WorkflowScheduler scheduler;

    // Inicializar os dados no scheduler
    scheduler.processed_texts = readCsvColumn(csv_filename, column_to_process);

    if (scheduler.processed_texts.empty()) {
        std::cerr << "Nenhum dado lido ou coluna não encontrada. Abortando pipeline." << std::endl;
        return 1;
    }
    std::cout << "Total de " << scheduler.processed_texts.size() << " entradas lidas da coluna '" << column_to_process << "'." << std::endl;


    // 2. Definir as tarefas do workflow com suas prioridades e funções
    // Prioridades: menor número = maior prioridade
    scheduler.addTask(Task("CleanText", TaskType::TEXT_CLEANING, 10, cleanText));
    scheduler.addTask(Task("NormalizeText", TaskType::NORMALIZATION, 20, normalizeText));
    scheduler.addTask(Task("WordTokenization", TaskType::WORD_TOKENIZATION, 30, wordTokenization));
    scheduler.addTask(Task("BPETokenization", TaskType::BPE_TOKENIZATION, 40, bpeTokenization));
    scheduler.addTask(Task("PartitionTokens", TaskType::PARTITION_TOKENS, 50, partitionTokens));
    scheduler.addTask(Task("AddSpecialTokens", TaskType::ADD_SPECIAL_TOKENS, 60, addSpecialTokens));
    scheduler.addTask(Task("TokensToIndices", TaskType::TOKENS_TO_INDICES, 70, tokensToIndices));
    scheduler.addTask(Task("GenerateEmbeddings", TaskType::GENERATE_EMBEDDINGS, 80, generateEmbeddings));


    // 3. Definir as dependências conforme o grafo
    // Preprocessing
    scheduler.addDependency("NormalizeText", "CleanText");
    scheduler.addDependency("WordTokenization", "NormalizeText");
    scheduler.addDependency("BPETokenization", "WordTokenization");
    scheduler.addDependency("PartitionTokens", "BPETokenization");

    // Embeddings Generation
    scheduler.addDependency("AddSpecialTokens", "PartitionTokens");
    scheduler.addDependency("TokensToIndices", "AddSpecialTokens");
    scheduler.addDependency("GenerateEmbeddings", "TokensToIndices");


    // 4. Executar o workflow
    // É recomendado usar std::thread::hardware_concurrency() para determinar o número ideal
    // de threads, mas para este exemplo, um número fixo é usado.
    int num_worker_threads = 4; // Exemplo: 4 threads de worker
    std::cout << "\nIniciando o agendador com " << num_worker_threads << " threads de worker..." << std::endl;
    scheduler.run(num_worker_threads);

    std::cout << "\nPipeline de pré-processamento concluído." << std::endl;

    // Opcional: Mostrar o resultado do pré-processamento para algumas entradas
    std::cout << "\nExemplo de texto após o pré-processamento (apenas as primeiras 50 entradas):" << std::endl;
    for (size_t i = 0; i < std::min((size_t)50, scheduler.processed_texts.size()); ++i) {
        // Limita a exibição para 150 caracteres para melhor leitura no console
        std::cout << "  Entrada " << i+1 << ": " << scheduler.processed_texts[i].substr(0, std::min((size_t)150, scheduler.processed_texts[i].length())) << "..." << std::endl;
    }

    return 0;
}