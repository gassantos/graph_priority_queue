# Pipeline de Pré-processamento de Dados Jurídicos

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![codecov](https://codecov.io/gh/username/graph_priority_summary/branch/main/graph/badge.svg)](https://codecov.io/gh/username/graph_priority_summary)

Este projeto implementa um **pipeline modular de pré-processamento de dados jurídicos** em C++ utilizando um grafo de dependências para orquestração paralela de tarefas. O sistema processa documentos jurídicos através de múltiplas etapas de transformação, incluindo limpeza de texto, tokenização BPE, partição, adição de tokens especiais, conversão para índices e simulação de embeddings.

## 🏗️ Arquitetura do Sistema

O projeto foi **completamente modularizado** seguindo as melhores práticas de engenharia de software. O pipeline é estruturado como um grafo de dependências onde cada nó representa uma tarefa de processamento. O scheduler gerencia a execução paralela respeitando as dependências entre tarefas e suas prioridades.

### ⚡ Principais Características

- ✅ **Detecção automática de CPUs**: Utiliza automaticamente todas as threads disponíveis
- ✅ **Execução paralela e sequencial**: Comparação automática de performance
- ✅ **Compilação limpa**: Zero warnings com flags rigorosas de compilação
- ✅ **Arquitetura modular**: Código organizado em namespaces e módulos bem definidos
- ✅ **Sistema de build robusto**: Suporte a Makefile e CMake

### ⛓️ Fluxo do Pipeline

```
Dados CSV → CleanText → NormalizeText → WordTokenization → BPETokenization
                                                                ↓
GenerateEmbeddings ← TokensToIndices ← AddSpecialTokens ← PartitionTokens
```

## 🔎 Etapas do Pipeline Detalhadas

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

### 🔎 Arquitetura Modular

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

## :hammer_and_pick: Funcionalidades

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


## 🧪 Testes e Validação

- ✅ **Validação de entrada**: Verificação de arquivos CSV e colunas
- ✅ **Detecção de ciclos**: Validação do grafo de dependências
- ✅ **Gerenciamento de recursos**: Threads, mutexes, memória
- ✅ **Tratamento de erros**: Exceções e casos extremos
- ✅ **Compilação limpa**: Zero warnings com flags rigorosas



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
```

### 📊 Resultados Obtidos
```
⏱️  TEMPOS DE EXECUÇÃO:
  Pipeline Paralelo (Scheduler):     5.0324 segundos
  Pipeline Sequencial (Thread Única): 4.8970 segundos
  Pipeline Paralelo (Particionado):   0.9342 segundos

🚀 SPEEDUPS:
  Scheduler vs Sequencial:     0.9731x (PIOR)
  Particionado vs Sequencial:  5.2420x (MELHOR)
  Particionado vs Scheduler:   5.3871x (MELHOR)

📈 THROUGHPUT (documentos/segundo):
  Scheduler:     9532.5566
  Sequencial:    9796.2818
  Particionado:  51352.4276

🏆 PARTICIONAMENTO DE DADOS PARALELIZADO é a melhor estratégia para este fluxo de dados. ✅
```

## 🎯 **RESUMO FINAL**

O pipeline baseado em Grafo de Prioridades para pré-processamento de textos jurídicos foi **aprimorado com sucesso** para explorar paralelismo real através de **particionamento de dados**.

### **🚀 PRINCIPAIS IMPLEMENTAÇÕES**

#### **1. Modo Paralelo com Particionamento de Dados**
- **Método `runParallelPartitioned()`**: Divide os dados em chunks e processa cada chunk em paralelo
- **Particionamento inteligente**: Calcula automaticamente o tamanho ideal dos chunks baseado no número de workers
- **Processamento real em paralelo**: Cada worker processa um chunk completo independentemente
- **Speedup significativo**: Até **5.24x** mais rápido que o modo sequencial

#### **2. Três Cenários de Execução**
- **Sequencial**: Execução em thread única para baseline
- **Paralelo Tradicional**: Scheduler com grafo de dependências (limitado pela natureza linear do pipeline)
- **Paralelo Particionado**: Divisão dos dados para paralelismo real

#### **3. Comparação Automática de Performance**
- **Método `runFullComparison()`**: Executa os três cenários automaticamente
- **Análise detalhada**: Speedup, eficiência, throughput para cada modo
- **Validação de consistência**: Verifica se todos os modos produzem resultados idênticos
- **Recomendação automática**: Indica o melhor modo para o volume de dados


### **🏆 CONCLUSÃO**

O projeto foi bem-sucedido em todos os aspectos:

1. **✅ Paralelismo real implementado** através de particionamento de dados
2. **✅ Três modos de execução** validados e comparados
3. **✅ Performance significativamente melhorada** (5.24x speedup)
4. **✅ Estatísticas detalhadas** implementadas para todos os cenários
5. **✅ Código limpo e documentado** sem warnings de compilação
6. **✅ Validação completa** com testes de consistência e performance


## 📈 Roadmap e Próximos Passos

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
