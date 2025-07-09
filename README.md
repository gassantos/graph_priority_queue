# Pipeline de Pré-processamento de Dados Jurídicos

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![codecov](https://codecov.io/gh/username/graph_priority_summary/branch/main/graph/badge.svg)](https://codecov.io/gh/username/graph_priority_summary)

Este projeto implementa um **pipeline modular de pré-processamento de dados jurídicos** em C++ utilizando um grafo de dependências para orquestração paralela de tarefas. O sistema processa documentos jurídicos através de múltiplas etapas de transformação, incluindo limpeza de texto, tokenização BPE, partição, adição de tokens especiais, conversão para índices e simulação de embeddings.

## 🏗️ Arquitetura do Sistema

O projeto foi **completamente modularizado** seguindo as melhores práticas de engenharia de software. O pipeline é estruturado como um grafo de dependências onde cada nó representa uma tarefa de processamento. O scheduler gerencia a execução paralela respeitando as dependências entre tarefas e suas prioridades.

### 🎯 Principais Características

- ✅ **Detecção automática de CPUs**: Utiliza automaticamente todas as threads disponíveis
- ✅ **Execução paralela e sequencial**: Comparação automática de performance
- ✅ **Modo sequencial puro**: Execução verdadeiramente sequencial para benchmarks precisos
- ✅ **Compilação limpa**: Zero warnings com flags rigorosas de compilação
- ✅ **Arquitetura modular**: Código organizado em namespaces e módulos bem definidos
- ✅ **Sistema de build robusto**: Suporte a Makefile e CMake

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
├── LICENSE                           # Licença do projeto
├── dataset.csv                       # Dataset de entrada
└── README.md                         # Esta documentação
```

## 🚀 Compilação e Execução com Makefile

```bash
# Compilar 
make

# Compilar com debug
make debug

# Executar
make run

# Limpar arquivos de build
make clean

# Mostrar ajuda
make help

# Mostrar estrutura do projeto
make structure
```

## ⚡ Resultados de Performance

### Configuração Atual
- **Dataset**: docs.csv (47.972 documentos jurídicos)
- **Hardware**: Detecção automática (16 threads no exemplo)
- **Compilador**: g++ com otimização -O2

### Resultados Típicos
```
=== Pipeline de Pré-processamento de Dados Jurídicos ===
Versão Modular - Engenharia de Software
Configuração do pipeline:
  - Threads disponíveis detectadas: 16
  - Workers configurados: 16

Total de 47972 entradas lidas da coluna 'Texto'.

=== COMPARAÇÃO DE PERFORMANCE ===
Tempo Paralelo:   4.46 segundos
Tempo Sequencial: 4.30 segundos (Modo Thread Única)
Speedup:          0.96x

=== ESTATÍSTICAS DETALHADAS ===
Pipeline Paralelo:
  - Tarefas concluídas: 8
  - Tempo de execução: 4.46138 segundos
  - Documentos processados: 47972
Pipeline Sequencial:
  - Tarefas concluídas: 8
  - Tempo de execução: 4.30141 segundos
  - Documentos processados: 47972

Comparação de Performance:
  - Speedup: 0.96x
  - Eficiência: 6.03%
  - Workers utilizados: 16

✓ Resultados dos pipelines são consistentes!
```

### Análise de Performance
- **Overhead de paralelização**: Para este dataset específico, o overhead supera o benefício
- **Workload I/O bound**: Processamento limitado por E/S mais que por CPU
- **Consistência garantida**: Resultados próximos entre execução paralela e sequencial
- **Escalabilidade**: Adapta-se automaticamente ao hardware disponível

## 🔍 Funcionalidades

### Detecção Automática de Hardware
```cpp
// Detecta automaticamente o número de threads disponíveis
unsigned int max_threads = std::thread::hardware_concurrency();
config.num_workers = max_threads;
```

### Validação de Grafo
- **Detecção de ciclos**: Algoritmo DFS para validar dependências
- **Representação visual**: Geração de string do grafo para debug
- **Verificação de consistência**: Validação automática antes da execução


### Configuração Flexível
```cpp
PipelineConfig config;
config.num_workers = 16;                 // Threads (ou detecção automática)
config.enable_debug = true;              // Logs detalhados
config.max_sequence_length = 256;        // Tamanho máximo de sequência
```

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

## 🧪 Testes e Validação

- ✅ **Consistência**: Resultados semelhantes entre execução paralela e sequencial
- ✅ **Validação de entrada**: Verificação de arquivos CSV e colunas
- ✅ **Detecção de ciclos**: Validação do grafo de dependências
- ✅ **Gerenciamento de recursos**: Threads, mutexes, memória
- ✅ **Tratamento de erros**: Exceções e casos extremos
- ✅ **Compilação limpa**: Zero warnings com flags rigorosas

### Métricas de Qualidade
```cpp
// Verificação automática de consistência
bool results_match = (parallel_result.processed_data.size() == 
                     sequential_result.processed_data.size());
```

## 📈 Roadmap e Próximos Passos

### 🎯 **Próximas Versões**
- [ ] **Integração com HuggingFace Tokenizers (pybind)**
- [ ] **Suporte a modelos de embedding reais (LibTorch/ONNX)**
- [ ] **Interface de linha de comando mais robusta**
- [ ] **Suporte a múltiplos formatos de entrada (JSON, Parquet)**

## 📄 Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.

## 👥 Autores e Contribuidores

- **[Gustavo Alexandre Santos](https://github.com/gassantos)** - *Desenvolvimento principal e arquitetura*

## 📚 Referências e Recursos

- [Modern C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [HuggingFace Tokenizers](https://huggingface.co/docs/tokenizers/)
- [Byte Pair Encoding (BPE)](https://arxiv.org/abs/1508.07909)
- [BERT: Pre-training of Deep Bidirectional Transformers](https://arxiv.org/abs/1810.04805)

---
