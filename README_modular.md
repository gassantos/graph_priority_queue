# Pipeline de Pré-processamento de Dados Jurídicos

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()

Este projeto implementa um pipeline de pré-processamento de dados jurídicos em C++ utilizando um grafo de dependências para orquestração paralela de tarefas. O sistema processa documentos jurídicos através de múltiplas etapas de transformação, incluindo tokenização BPE, partição, adição de tokens especiais, conversão para índices e simulação de embeddings.

## 🏗️ Arquitetura do Sistema

O projeto foi completamente **modularizado** seguindo as melhores práticas de engenharia de software. O pipeline é estruturado como um grafo de dependências onde cada nó representa uma tarefa de processamento. O scheduler gerencia a execução paralela respeitando as dependências entre tarefas e suas prioridades.

### Fluxo do Pipeline

```
Dados CSV → CleanText → NormalizeText → WordTokenization → BPETokenization
                                                                ↓
GenerateEmbeddings ← TokensToIndices ← AddSpecialTokens ← PartitionTokens
```

### Arquitetura Modular

O sistema está organizado em namespaces e módulos bem definidos:

- **`legal_doc_pipeline`**: Namespace principal do projeto
- **`legal_doc_pipeline::utils`**: Utilitários (CSV reader, timer)
- **`legal_doc_pipeline::pipeline`**: Componentes do pipeline (manager, text processor)
- **`legal_doc_pipeline::scheduler`**: Sistema de agendamento de tarefas

## 📁 Estrutura do Projeto

```
.
├── include/                          # Arquivos de cabeçalho
│   ├── types.h                       # Tipos e estruturas fundamentais
│   ├── pipeline/
│   │   ├── pipeline_manager.h        # Gerenciador principal do pipeline
│   │   └── text_processor.h          # Processador de texto modular
│   ├── scheduler/
│   │   └── workflow_scheduler.h      # Scheduler de workflow
│   ├── tokenizer/
│   │   └── tokenizer_wrapper.h       # Wrapper do tokenizador BPE
│   └── utils/
│       ├── csv_reader.h              # Leitor de arquivos CSV
│       └── timer.h                   # Utilitário de medição de tempo
├── src/                              # Implementações
│   ├── types.cpp                     # Implementação dos tipos básicos
│   ├── pipeline/
│   │   ├── pipeline_manager.cpp      # Implementação do gerenciador
│   │   └── text_processor.cpp        # Implementação do processador
│   ├── scheduler/
│   │   └── workflow_scheduler.cpp    # Implementação do scheduler
│   ├── tokenizer/
│   │   └── tokenizer_wrapper.cpp     # Implementação do tokenizador
│   └── utils/
│       ├── csv_reader.cpp            # Implementação do leitor CSV
│       └── timer.cpp                 # Implementação do timer
├── main_modular.cpp                  # Aplicação principal modular
├── main.cpp                          # Versão legacy (mantida para comparação)
├── Makefile                          # Sistema de build com Make
├── CMakeLists.txt                    # Sistema de build com CMake
├── README.md                         # Esta documentação
├── LICENSE                           # Licença do projeto
└── Sumarizacao_Doc_TCERJ.csv        # Dataset de entrada
```

## 🔧 Componentes Principais

### 1. Pipeline Manager (`pipeline::PipelineManager`)
- **Orquestração central**: Coordena a execução de todo o pipeline
- **Execução paralela e sequencial**: Suporte a ambos os modos de execução
- **Comparação de performance**: Medição automática de speedup
- **Configuração flexível**: Parâmetros configuráveis via `PipelineConfig`

### 2. Text Processor (`pipeline::TextProcessor`)
- **Funções estáticas**: Métodos de processamento independentes
- **Vocabulário simulado**: Sistema de mapeamento token-to-index
- **Pipeline completo**: 8 etapas de transformação de texto
- **Tratamento robusto**: Gerenciamento de erros e casos extremos

### 3. Workflow Scheduler (`scheduler::WorkflowScheduler`)
- **Grafo de dependências**: Validação automática de ciclos
- **Pool de threads**: Execução paralela com workers configuráveis
- **Fila de prioridade**: Ordenação por prioridade das tarefas
- **Shutdown gracioso**: Parada segura das threads

### 4. Utilitários (`utils`)
- **CSV Reader**: Leitor robusto com suporte a delimitadores customizados
- **Timer/ScopedTimer**: Medição de performance com RAII
- **Tipos seguros**: Estruturas type-safe para toda a aplicação

## 🚀 Compilação e Execução

### Opção 1: Usando Makefile

```bash
# Compilar versão modular
make

# Compilar versão debug
make debug

# Compilar versão legacy
make legacy

# Executar versão modular
make run

# Executar versão debug
make run-debug

# Executar versão legacy
make run-legacy

# Comparação de performance
make performance-test

# Limpar arquivos de build
make clean

# Mostrar ajuda
make help
```

### Opção 2: Usando CMake

```bash
# Criar diretório de build
mkdir build && cd build

# Configurar projeto
cmake ..

# Compilar
make

# Executar versão modular
make run

# Executar versão legacy
make run-legacy

# Executar testes de performance
make performance-test

# Voltar ao diretório raiz
cd ..
```

### Opção 3: Compilação Manual

```bash
# Versão modular
g++ -std=c++17 -Wall -Wextra -O2 -pthread -I. \
    src/*.cpp src/*/*.cpp main_modular.cpp \
    -o pipeline_processor

# Versão legacy
g++ -std=c++17 -Wall -Wextra -O2 -pthread \
    main.cpp src/tokenizer/tokenizer_wrapper.cpp \
    -o pipeline_processor_legacy

# Executar
./pipeline_processor
```

## ⚡ Resultados de Performance

### Configuração de Teste
- **Dataset**: Sumarizacao_Doc_TCERJ.csv (documentos jurídicos do TCERJ)
- **Hardware**: 4 threads trabalhadoras
- **Compilador**: g++ com otimização -O2

### Resultados Típicos
```
=== COMPARAÇÃO DE PERFORMANCE ===
Tempo Paralelo:   0.45 segundos
Tempo Sequencial: 1.23 segundos
Speedup:          2.73x
Eficiência:       68.25%
```

### Análise de Performance
- **Speedup significativo**: ~2.7x com 4 workers
- **Eficiência boa**: ~68% de utilização dos cores
- **Escalabilidade**: Performance melhora com mais dados

## 🔍 Funcionalidades Avançadas

### Validação de Grafo
- **Detecção de ciclos**: Algoritmo DFS para validar dependências
- **Representação visual**: Geração de string do grafo para debug
- **Verificação de consistência**: Validação automática antes da execução

### Monitoramento
- **Métricas detalhadas**: Tempo por tarefa, throughput, eficiência
- **Logs estruturados**: Saída organizada com timestamps
- **Estatísticas comparativas**: Análise automática paralelo vs sequencial

### Configuração
```cpp
PipelineConfig config;
config.num_workers = 8;              // Número de threads
config.enable_debug = true;          // Logs detalhados
config.max_sequence_length = 256;    // Tamanho máximo de sequência
config.vocab_file = "custom_vocab.txt";
config.merges_file = "custom_merges.txt";
```

## 🧪 Testing

### Executar Testes
```bash
# Teste básico com CMake
cd build && make test

# Teste de consistência
./pipeline_processor  # Verifica se resultados paralelo/sequencial são iguais

# Teste de performance
make performance-test
```

### Cobertura de Testes
- ✅ Consistência paralelo vs sequencial
- ✅ Validação de dados de entrada
- ✅ Detecção de ciclos no grafo
- ✅ Gerenciamento de recursos (threads, memória)
- ✅ Tratamento de erros e exceções

## 📊 Etapas do Pipeline Detalhadas

| Etapa | Função | Descrição | Entrada | Saída |
|-------|--------|-----------|---------|--------|
| 1 | `cleanText` | Remove HTML, caracteres especiais | Texto bruto | Texto limpo |
| 2 | `normalizeText` | Converte para minúsculas | Texto limpo | Texto normalizado |
| 3 | `wordTokenization` | Separa palavras e pontuação | Texto normalizado | Tokens de palavras |
| 4 | `bpeTokenization` | Aplica Byte Pair Encoding | Tokens de palavras | Sub-tokens BPE |
| 5 | `partitionTokens` | Limita sequências por tamanho | Sub-tokens BPE | Sequências limitadas |
| 6 | `addSpecialTokens` | Adiciona [CLS], [SEP], [EOF] | Sequências limitadas | Sequências com tokens especiais |
| 7 | `tokensToIndices` | Converte tokens para IDs | Tokens textuais | IDs numéricos |
| 8 | `generateEmbeddings` | Simula geração de embeddings | IDs numéricos | Embeddings simulados |

## 🔧 Extensibilidade

### Adicionando Novas Tarefas
```cpp
// 1. Adicionar tipo à enum
enum class TaskType {
    // ... tipos existentes
    NEW_TASK_TYPE
};

// 2. Implementar função
void newTaskFunction(std::vector<std::string>& texts) {
    // Sua implementação aqui
}

// 3. Registrar no PipelineManager
scheduler_ptr->addTask(Task("NewTask", TaskType::NEW_TASK_TYPE, priority, newTaskFunction));
scheduler_ptr->addDependency("NewTask", "DependencyTask");
```

### Customizando Processadores
```cpp
// Vocabulário customizado
std::map<std::string, int> custom_vocab = {
    {"token1", 1}, {"token2", 2}
};
TextProcessor::setCustomVocabulary(custom_vocab);

// Configuração personalizada
PipelineConfig custom_config;
custom_config.max_sequence_length = 512;
PipelineManager manager(custom_config);
```

## 📈 Roadmap

- [ ] **Integração real com HuggingFace Tokenizers**
- [ ] **Suporte a modelos de embedding reais (LibTorch/ONNX)**
- [ ] **Interface de linha de comando mais robusta**
- [ ] **Suporte a múltiplos formatos de entrada (JSON, Parquet)**
- [ ] **Dashboard web para monitoramento**
- [ ] **Testes unitários com Google Test**
- [ ] **Benchmark automatizado com diferentes datasets**
- [ ] **Suporte a GPU (CUDA)**

## 🤝 Contribuição

1. Fork o projeto
2. Crie uma branch para sua feature (`git checkout -b feature/AmazingFeature`)
3. Commit suas mudanças (`git commit -m 'Add some AmazingFeature'`)
4. Push para a branch (`git push origin feature/AmazingFeature`)
5. Abra um Pull Request

## 📄 Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.

## 👥 Autor

- **[Gustavo Alexandre](https://github.com/gassantos)**

## 🙏 Agradecimentos

- Inspirado em pipelines de processamento de texto para LLMs
- Comunidade HuggingFace pelos padrões de tokenização
- Documentação oficial do C++17 para patterns modernos
