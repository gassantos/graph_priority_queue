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
#include <numeric>      // Para std::accumulate

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
    //std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    std::cout << "  [Task] CleanText concluído." << std::endl;
}

// 2. Normalização (Padronização)
void normalizeText(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando NormalizeText..." << std::endl;
    for (std::string& text : texts) {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char c){ return std::tolower(c); });
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    std::cout << "  [Task] NormalizeText concluído." << std::endl;
}

// 3. Tokenização de palavras (aprimorada com regex para pontuação)
void wordTokenization(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando WordTokenization (aprimorado)..." << std::endl;
    // Expressão regular para encontrar palavras e pontuação.
    // [a-zA-Z0-9À-ÿ]+: uma ou mais letras/números/acentuados
    // |: OU
    // [.,!?;:"'()\[\]{}] : qualquer um dos caracteres de pontuação comuns
    std::regex word_punct_regex("[a-zA-Z0-9À-ÿ]+|[.,!?;:\"'()\\[\\]{}]", std::regex::ECMAScript | std::regex::collate);

    for (std::string& text : texts) {
        std::vector<std::string> tokens;
        auto words_begin = std::sregex_iterator(text.begin(), text.end(), word_punct_regex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            std::string token = match.str();
            // Opcional: Filtra tokens vazios ou de apenas espaço se a regex não for perfeita
            if (!token.empty() && !std::all_of(token.begin(), token.end(), ::isspace)) {
                tokens.push_back(token);
            }
        }
        
        // Reconstrói o texto como uma string de tokens separados por espaço
        // Isso mantém a compatibilidade da assinatura da função para as próximas etapas
        // Em um pipeline real, a saída poderia ser std::vector<std::vector<std::string>>
        // para o próximo estágio consumir diretamente.
        text = std::accumulate(tokens.begin(), tokens.end(), std::string(),
                               [](const std::string& a, const std::string& b) {
                                   return a.empty() ? b : a + " " + b;
                               });
    }
    printf("  [Task] WordTokenization concluído.\n");
}

// 4. Tokenizador BPE (Byte Pair Encoding) - Simulado
void bpeTokenization(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando BPETokenization (simulado)..." << std::endl;
    // Implementação real exigiria uma biblioteca BPE.
    //std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    printf("  [Task] BPETokenization concluído.\n");
}

// 5. Particionar texto em tokens - Simulado
void partitionTokens(std::vector<std::string>& texts) {
    printf("  [Task] Executando PartitionTokens (simulado)...\n");
    // Em um cenário real, dividiria as sequências de tokens em chunks.
    //std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    printf("  [Task] PartitionTokens concluído.\n");
}

// 6. Adicionar tokens especiais (CLS, SEP, EOF) - Simulado
void addSpecialTokens(std::vector<std::string>& texts) {
    printf("  [Task] Executando AddSpecialTokens...\n");
    for (std::string& text : texts) {
        text = "[CLS] " + text + " [SEP] [EOF]";
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    printf("[Task] AddSpecialTokens concluído.\n");
}

// 7. Conversão de tokens para índices - Simulado
void tokensToIndices(std::vector<std::string>& texts) {
    printf("[Task] Executando TokensToIndices (simulado)...\n");
    // Em uma implementação real, usaria um vocabulário para mapear tokens para IDs.
    //std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    printf("[Task] TokensToIndices concluído.\n");
}

// 8. Geração de vetores de embeddings - Simulado
void generateEmbeddings(std::vector<std::string>& texts) {
    printf("[Task] Executando GenerateEmbeddings (simulado)...\n");
    // Use um modelo de embedding e gera vetores em linguagem C++
    // Em uma implementação real, isso envolveria uma biblioteca de machine learning.
    // Simula um atraso para a operação
    // Aqui, apenas simula a operação com um atraso.

    //std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simula trabalho
    printf("[Task] GenerateEmbeddings concluído.\n");
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
                printf("Tarefa inicial '%s' adicionada à fila de prontos.\n", pair.second.id.c_str());
            }
        }
        cv_tasks_ready.notify_all(); // Notifica que tarefas iniciais estão prontas
    }

    void markTaskCompleted(const std::string& task_id) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        Task& completed_task = tasks.at(task_id);
        completed_task.is_completed = true;
        completed_task_count++; // Incrementa o contador global de tarefas concluídas
        std::cout << "Tarefa '" << task_id << "' finalizada! Total concluídas: " << completed_task_count.load() << std::endl;

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
                    std::cout << "Worker encerrando: todas as tarefas concluídas. (ID thread: " << std::this_thread::get_id() << ")\n";
                    break; // Sai do loop do worker
                }

                if (!ready_queue.empty()) {
                    current_task_ptr = ready_queue.top(); // Pega o ponteiro
                    ready_queue.pop(); // Remove o ponteiro da fila
                    task_found = true;
                    std::cout << "Worker (ID: " << std::this_thread::get_id()
                               << ") pegou a tarefa: " << current_task_ptr->id << std::endl;
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
        // processed_texts é inicializado e preenchido antes desta chamada
        // completed_task_count é resetado no construtor
        
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
        printf("Todos os workers terminaram a execução.\n");
    }
};

// --- Novo Método para Execução Sequencial ---

void run_sequential_pipeline(std::vector<std::string>& texts_data) {
    printf("\n--- Iniciando Pipeline Sequencial ---\n");
    auto start_seq = std::chrono::high_resolution_clock::now();

    // Resetar o contador de tarefas completas (para o caso de ser rodado após o paralelo)
    // Embora não seja estritamente necessário para o pipeline sequencial, mantém a consistência
    // com o estado das funções de Task que imprimem o "Task concluído"
    std::atomic<int> sequential_task_count(0);

    // Execução das tarefas em sequência
    cleanText(texts_data);
    sequential_task_count++;
    printf("Tarefa 'CleanText' finalizada! Total concluídas: %d\n", sequential_task_count.load());

    normalizeText(texts_data);
    sequential_task_count++;
    printf("Tarefa 'NormalizeText' finalizada! Total concluídas: %d\n", sequential_task_count.load());

    wordTokenization(texts_data);
    sequential_task_count++;
    printf("Tarefa 'WordTokenization' finalizada! Total concluídas: %d\n", sequential_task_count.load());

    bpeTokenization(texts_data);
    sequential_task_count++;
    std::cout << "Tarefa 'BPETokenization' finalizada! Total concluídas: " << sequential_task_count.load() << std::endl;

    partitionTokens(texts_data);
    sequential_task_count++;
    std::cout << "Tarefa 'PartitionTokens' finalizada! Total concluídas: " << sequential_task_count.load() << std::endl;

    addSpecialTokens(texts_data);
    sequential_task_count++;
    std::cout << "Tarefa 'AddSpecialTokens' finalizada! Total concluídas: " << sequential_task_count.load() << std::endl;

    tokensToIndices(texts_data);
    sequential_task_count++;
    std::cout << "Tarefa 'TokensToIndices' finalizada! Total concluídas: " << sequential_task_count.load() << std::endl;

    generateEmbeddings(texts_data);
    sequential_task_count++;
    std::cout << "Tarefa 'GenerateEmbeddings' finalizada! Total concluídas: " << sequential_task_count.load() << std::endl;
    
    auto end_seq = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seq = end_seq - start_seq;
    std::cout << "--- Pipeline Sequencial Concluído ---\n";
    std::cout << "Total de tarefas concluídas (sequencial): " << sequential_task_count.load() << std::endl;
    std::cout << "Tempo total de execução (sequencial): " << elapsed_seq.count() << " segundos.\n";
}


// --- 4. Função Principal (main) ---

int main() {
    std::cout << "Iniciando pipeline de pré-processamento de dados jurídicos.\n";

    // 1. Carregar os dados
    std::string csv_filename = "Sumarizacao_Doc_TCERJ.csv";
    std::string column_to_process = "Texto";
    std::cout << "Lendo coluna '" << column_to_process << "' do arquivo '" << csv_filename << "'...\n";

    std::vector<std::string> initial_texts = readCsvColumn(csv_filename, column_to_process);

    if (initial_texts.empty()) {
        std::cerr << "Nenhum dado lido ou coluna não encontrada. Abortando pipeline.\n";
        return 1;
    }
    std::cout << "Total de " << initial_texts.size() << " entradas lidas da coluna '" << column_to_process << "'.\n";

    // --- Execução do Pipeline Paralelo (com grafo e threads) ---
    // Cria uma cópia dos dados para cada execução para garantir que as operações não afetem a outra
    std::vector<std::string> texts_for_parallel = initial_texts;
    WorkflowScheduler parallel_scheduler;
    parallel_scheduler.processed_texts = texts_for_parallel; // Atribui a cópia dos dados

    // 2. Definir as tarefas do workflow com suas prioridades e funções (para o scheduler paralelo)
    // Prioridades: menor número = maior prioridade
    parallel_scheduler.addTask(Task("CleanText", TaskType::TEXT_CLEANING, 10, cleanText));
    parallel_scheduler.addTask(Task("NormalizeText", TaskType::NORMALIZATION, 20, normalizeText));
    parallel_scheduler.addTask(Task("WordTokenization", TaskType::WORD_TOKENIZATION, 30, wordTokenization));
    parallel_scheduler.addTask(Task("BPETokenization", TaskType::BPE_TOKENIZATION, 40, bpeTokenization));
    parallel_scheduler.addTask(Task("PartitionTokens", TaskType::PARTITION_TOKENS, 50, partitionTokens));
    parallel_scheduler.addTask(Task("AddSpecialTokens", TaskType::ADD_SPECIAL_TOKENS, 60, addSpecialTokens));
    parallel_scheduler.addTask(Task("TokensToIndices", TaskType::TOKENS_TO_INDICES, 70, tokensToIndices));
    parallel_scheduler.addTask(Task("GenerateEmbeddings", TaskType::GENERATE_EMBEDDINGS, 80, generateEmbeddings));

    // 3. Definir as dependências conforme o grafo (para o scheduler paralelo)
    // Preprocessing
    parallel_scheduler.addDependency("NormalizeText", "CleanText");
    parallel_scheduler.addDependency("WordTokenization", "NormalizeText");
    parallel_scheduler.addDependency("BPETokenization", "WordTokenization");
    parallel_scheduler.addDependency("PartitionTokens", "BPETokenization");

    // Embeddings Generation
    parallel_scheduler.addDependency("AddSpecialTokens", "PartitionTokens");
    parallel_scheduler.addDependency("TokensToIndices", "AddSpecialTokens");
    parallel_scheduler.addDependency("GenerateEmbeddings", "TokensToIndices");

    int num_worker_threads = 4; // Exemplo: 4 threads de worker
    auto start_parallel = std::chrono::high_resolution_clock::now();
    std::cout << "\n--- Iniciando Pipeline Paralelo com " << num_worker_threads
              << " threads de worker ---\n";
    parallel_scheduler.run(num_worker_threads);
    auto end_parallel = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_parallel = end_parallel - start_parallel;
    std::cout << "--- Pipeline Paralelo Concluído ---\n";
    std::cout << "Tempo total de execução (paralelo): " << elapsed_parallel.count() << " segundos.\n";

    // --- Execução do Pipeline Sequencial ---
    // Cria uma nova cópia dos dados para o pipeline sequencial
    std::vector<std::string> texts_for_sequential = initial_texts;
    run_sequential_pipeline(texts_for_sequential);

    // --- Exemplo de texto após o pré-processamento (para ambos os pipelines) ---
    std::cout << "\n--- Exemplo de texto após o pré-processamento (apenas as primeiras 5 entradas para comparação) ---\n";
    // Opcional: Mostrar o resultado do pré-processamento para algumas entradas do pipeline PARALELO
    std::cout << "\nResultado do Pipeline Paralelo (primeiras 5 entradas):\n";
    for (size_t i = 0; i < std::min((size_t)5, parallel_scheduler.processed_texts.size()); ++i) {
        printf("  Entrada %zu: %.150s...\n", i + 1, parallel_scheduler.processed_texts[i].c_str());
    }

    // Opcional: Mostrar o resultado do pré-processamento para algumas entradas do pipeline SEQUENCIAL
    std::cout << "\nResultado do Pipeline Sequencial (primeiras 5 entradas):\n";
    for (size_t i = 0; i < std::min((size_t)5, texts_for_sequential.size()); ++i) {
        printf("  Entrada %zu: %.150s...\n", i + 1, texts_for_sequential[i].c_str());
    }
    
    return 0;
}