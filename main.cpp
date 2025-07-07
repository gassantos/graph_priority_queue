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

// Importa as funções do tokenizer_wrapper
#include "tokenizer_wrapper.h"

// --- Variáveis globais para tokensToIndices ---
std::map<std::string, int> vocabulary;
const int UNK_ID_TOKENS_TO_INDICES = 0; // ID para tokens desconhecidos

// Função para inicializar o vocabulário usado em tokensToIndices
void initialize_vocabulary() {
    static bool initialized = false;
    if (initialized) return;
    
    // Vocabulário simulado para mapeamento de tokens para IDs
    vocabulary = {
        // Tokens especiais
        {"[CLS]", 101}, {"[SEP]", 102}, {"[EOF]", 103}, {"[UNK]", 0},
        // Tokens comuns do domínio jurídico
        {"o", 1}, {"e", 2}, {"a", 3}, {"do", 4}, {"da", 5},
        {"um", 6}, {"documento", 7}, {"visa", 8}, {"apresentar", 9},
        {"fluxo", 10}, {"tarefas", 11}, {"para", 12}, {"sumarização", 13},
        {"texto", 14}, {"documentos", 15}, {"jurídicos", 16}, {"dados", 17},
        {"processo", 18}, {"tribunal", 19}, {"justiça", 20}, {"lei", 21},
        {"artigo", 22}, {"código", 23}, {"civil", 24}, {"penal", 25}
    };
    
    initialized = true;
}

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

// 4. Tokenizador BPE (Byte Pair Encoding)
// Versão simplificada para funcionar com o sistema de tarefas atual
void bpeTokenization(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando BPETokenization..." << std::endl;
    
    // Cria um tokenizador temporário para demonstração
    try {
        TokenizerWrapper tokenizer("vocab.txt", "merges.txt");
        
        for (std::string& text : texts) {
            // Simula a tokenização BPE
            auto encoding = tokenizer.tokenize_and_add_special_tokens(text);
            // Para manter compatibilidade com o pipeline, convertemos de volta para string
            // Em um cenário real, você manteria os encodings em uma estrutura separada
            std::string token_representation = "[CLS] ";
            for (const auto& token : encoding.tokens) {
                if (token.text != "[CLS]" && token.text != "[SEP]" && token.text != "[EOF]") {
                    token_representation += token.text + " ";
                }
            }
            token_representation += "[SEP]";
            text = token_representation;
        }
    } catch (const std::exception& e) {
        std::cerr << "Erro durante a tokenização: " << e.what() << std::endl;
    }
    
    std::cout << "  [Task] BPETokenization concluído." << std::endl;
}

// 4.1. Versão avançada da tokenização BPE (para uso direto)
void bpeTokenizationAdvanced(TokenizerWrapper& tokenizer_wrapper, const std::vector<std::string>& input_texts, std::vector<hf_tokenizers::Encoding>& encoded_outputs) {
    std::cout << "  [Task] Executando BPETokenization (versão avançada)..." << std::endl;
    encoded_outputs.clear(); // Limpa saídas anteriores
    for (const std::string& text : input_texts) {
        try {
            encoded_outputs.push_back(tokenizer_wrapper.tokenize_and_add_special_tokens(text));
        } catch (const std::exception& e) {
            std::cerr << "Erro durante a tokenização: " << e.what() << std::endl;
            // Decide como lidar com o erro: pular, lançar novamente, logar.
        }
    }
    std::cout << "  [Task] BPETokenization (versão avançada) concluído. Gerados " << encoded_outputs.size() << " encodings." << std::endl;
}

// 5. Particionar texto em tokens
void partitionTokens(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando PartitionTokens..." << std::endl;
    const size_t MAX_SEQUENCE_LENGTH = 128; // Exemplo de tamanho máximo da sequência de tokens

    std::vector<std::string> partitioned_texts;

    for (const std::string& text_tokens_str : texts) {
        std::istringstream iss(text_tokens_str);
        std::string token;
        std::vector<std::string> tokens;
        
        // Extrai todos os tokens da string
        while (iss >> token) {
            tokens.push_back(token);
        }

        if (tokens.size() > MAX_SEQUENCE_LENGTH) {
            // Trunca a sequência para o tamanho máximo
            std::vector<std::string> truncated_tokens(tokens.begin(), tokens.begin() + MAX_SEQUENCE_LENGTH);
            
            // Reconstrói a string truncada
            std::string truncated_str = "";
            for (size_t i = 0; i < truncated_tokens.size(); ++i) {
                truncated_str += truncated_tokens[i];
                if (i < truncated_tokens.size() - 1) {
                    truncated_str += " ";
                }
            }
            partitioned_texts.push_back(truncated_str);
            // Em uma implementação real, os chunks restantes também seriam adicionados como novas entradas
            // ou armazenados em uma estrutura de dados de múltiplas sequências por documento.
            // Aqui, por simplificação, apenas o primeiro chunk é mantido.
        } else {
            partitioned_texts.push_back(text_tokens_str); // Mantém a sequência como está
        }
    }
    texts = partitioned_texts; // Atualiza o vetor original
    std::cout << "  [Task] PartitionTokens concluído." << std::endl;
}


// 6. Adicionar tokens especiais (CLS, SEP, EOF) - Versão compatível com tokens de texto
void addSpecialTokens(std::vector<std::string>& texts) {
    std::cout << "  [Task] Executando AddSpecialTokens..." << std::endl;
    
    for (std::string& text : texts) {
        // Como os dados já podem conter [CLS] e [SEP] da tokenização BPE,
        // vamos adicionar apenas [EOF] no final se não estiver presente
        if (text.find("[EOF]") == std::string::npos) {
            text += " [EOF]";
        }
        
        // Garante que há [CLS] no início se não estiver presente
        if (text.find("[CLS]") != 0) {
            text = "[CLS] " + text;
        }
        
        // Garante que há [SEP] antes do [EOF] se não estiver presente
        if (text.find("[SEP]") == std::string::npos) {
            // Insere [SEP] antes do [EOF]
            size_t eof_pos = text.find("[EOF]");
            if (eof_pos != std::string::npos) {
                text.insert(eof_pos, "[SEP] ");
            } else {
                text += " [SEP]";
            }
        }
    }
    
    std::cout << "  [Task] AddSpecialTokens concluído." << std::endl;
}


// 7. Conversão de tokens para índices (compatível com tokens de texto)
void tokensToIndices(std::vector<std::string>& texts) {
    std::cout << "[Task] Executando TokensToIndices (simulado)..." << std::endl;
    
    // Assegura que o vocabulário esteja inicializado
    initialize_vocabulary(); 

    for (std::string& text_tokens_str : texts) {
        std::vector<int> final_indexed_sequence;
        std::istringstream iss(text_tokens_str);
        std::string token_str; 

        while (iss >> token_str) {
            // Busca o token no vocabulário
            auto it = vocabulary.find(token_str); 
            if (it != vocabulary.end()) {
                final_indexed_sequence.push_back(it->second);
            } else {
                final_indexed_sequence.push_back(UNK_ID_TOKENS_TO_INDICES); // Token não encontrado no vocabulário
            }
        }

        // Converte a sequência de IDs para string
        text_tokens_str = ""; 
        if (!final_indexed_sequence.empty()) {
            for (size_t i = 0; i < final_indexed_sequence.size(); ++i) {
                text_tokens_str += std::to_string(final_indexed_sequence[i]);
                if (i < final_indexed_sequence.size() - 1) {
                    text_tokens_str += " ";
                }
            }
        }
    }
    std::cout << "[Task] TokensToIndices concluído." << std::endl;
}

// 8. Geração de vetores de embeddings - Simulado (Aprimorado com placeholders)
void generateEmbeddings(std::vector<std::string>& texts) {
    std::cout << "[Task] Executando GenerateEmbeddings (simulado - gerando placeholders de embeddings)...\n";
    // Em uma implementação real, esta função receberia os IDs numéricos (int)
    // e passaria por um modelo de embedding (LibTorch, ONNX Runtime) para
    // gerar vetores densos de float.
    // O tipo de retorno idealmente seria std::vector<std::vector<float>>.

    // Para manter a assinatura std::vector<std::string>&, vamos substituir
    // o conteúdo da string por um placeholder para simular que o texto
    // foi transformado em um embedding.
    for (size_t i = 0; i < texts.size(); ++i) {
        texts[i] = "EMBEDDED_DOCUMENT_" + std::to_string(i + 1); // Ex: "EMBEDDED_DOCUMENT_1"
    }
    std::cout << "[Task] GenerateEmbeddings concluído.\n";
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

    #ifdef DEBUG
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
    #endif
    
    return 0;
}