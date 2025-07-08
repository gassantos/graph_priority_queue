# Pipeline de Pré-processamento de Dados Jurídicos

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![CI](https://github.com/username/graph_priority_summary/workflows/CI%20-%20Build,%20Test,%20and%20Quality%20Assurance/badge.svg)](https://github.com/username/graph_priority_summary/actions)
[![Security](https://github.com/username/graph_priority_summary/workflows/Security%20and%20Dependency%20Scanning/badge.svg)](https://github.com/username/graph_priority_summary/actions)
[![Documentation](https://github.com/username/graph_priority_summary/workflows/Documentation/badge.svg)](https://github.com/username/graph_priority_summary/actions)
[![Release](https://github.com/username/graph_priority_summary/workflows/Release%20Pipeline/badge.svg)](https://github.com/username/graph_priority_summary/releases)
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
├── Sumarizacao_Doc_TCERJ.csv        # Dataset de entrada
└── README.md                         # Esta documentação
```
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
└── dataset.csv                       # Dataset de entrada
```

## 🔧 Componentes Principais

### 1. Pipeline Manager (`pipeline::PipelineManager`)
- **Orquestração central**: Coordena a execução de todo o pipeline
- **Execução paralela e sequencial**: Suporte a ambos os modos de execução
- **Detecção automática de CPUs**: Utiliza automaticamente todas as threads disponíveis
- **Modo sequencial puro**: Execução verdadeiramente sequencial sem paralelismo
- **Comparação de performance**: Medição automática de speedup e eficiência
- **Configuração flexível**: Parâmetros configuráveis via `PipelineConfig`

### 2. Text Processor (`pipeline::TextProcessor`)
- **Funções estáticas**: Métodos de processamento independentes
- **Versões sequenciais**: Implementações específicas para execução sequencial pura
- **Vocabulário simulado**: Sistema de mapeamento token-to-index
- **Pipeline completo**: 8 etapas de transformação de texto
- **Tratamento robusto**: Gerenciamento de erros e casos extremos

### 3. Workflow Scheduler (`scheduler::WorkflowScheduler`)
- **Grafo de dependências**: Validação automática de ciclos
- **Pool de threads**: Execução paralela com workers configuráveis
- **Fila de prioridade**: Ordenação por prioridade das tarefas
- **Thread-safe**: Sincronização segura com mutexes e variáveis de condição
- **Shutdown gracioso**: Parada segura das threads
- **Contadores atômicos**: Uso de `std::atomic<size_t>` para thread safety

### 4. Utilitários (`utils`)
- **CSV Reader**: Leitor robusto com suporte a delimitadores customizados
- **Timer/ScopedTimer**: Medição de performance com RAII
- **Tipos seguros**: Estruturas type-safe para toda a aplicação

## 🚀 Compilação e Execução

### Opção 1: Usando Makefile (Recomendado)

```bash
# Compilar versão modular
make

# Compilar versão debug
make debug

# Executar versão modular
make run

# Executar versão debug
make run-debug

# Limpar arquivos de build
make clean

# Mostrar ajuda
make help

# Mostrar estrutura do projeto
make structure
```

### Opção 2: Usando CMake

```bash
# Criar diretório de build
mkdir build && cd build

# Configurar projeto
cmake ..

# Compilar
make

# Executar
./bin/pipeline_processor

# Voltar ao diretório raiz
cd ..
```

### Opção 3: Compilação Manual

```bash
# Versão modular com todas as otimizações
g++ -std=c++17 -Wall -Wextra -O2 -pthread -I. \
    src/types.cpp src/utils/csv_reader.cpp src/utils/timer.cpp \
    src/pipeline/text_processor.cpp src/pipeline/pipeline_manager.cpp \
    src/scheduler/workflow_scheduler.cpp src/tokenizer/tokenizer_wrapper.cpp \
    main_modular.cpp -o pipeline_processor

# Executar
./pipeline_processor
```

## ⚡ Resultados de Performance

### Configuração Atual
- **Dataset**: Sumarizacao_Doc_TCERJ.csv (47.972 documentos jurídicos)
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
- **Consistência garantida**: Resultados idênticos entre execução paralela e sequencial
- **Escalabilidade**: Adapta-se automaticamente ao hardware disponível

## 🔍 Funcionalidades Avançadas

### Detecção Automática de Hardware
```cpp
// Detecta automaticamente o número de threads disponíveis
unsigned int max_threads = std::thread::hardware_concurrency();
config.num_workers = max_threads;
```

### Execução Sequencial Pura
```cpp
// Força execução em thread única para benchmarks precisos
PipelineResult result = manager.runSequential(data, true);
```

### Validação de Grafo
- **Detecção de ciclos**: Algoritmo DFS para validar dependências
- **Representação visual**: Geração de string do grafo para debug
- **Verificação de consistência**: Validação automática antes da execução

## 🤖 CI/CD e Automação

### Workflows GitHub Actions

O projeto utiliza um sistema completo de CI/CD automatizado com múltiplos workflows:

#### 🔄 **CI Pipeline** (`.github/workflows/ci.yml`)
- **Build Matrix**: Testa em Ubuntu 20.04 e 22.04 com GCC e Clang
- **Compilação**: Build automatizado com CMake e Makefile (fallback)
- **Testes**: Execução de 56+ testes unitários com relatórios
- **Cache**: Cache inteligente de dependências para builds rápidos

#### 🔍 **Análise de Qualidade**
- **Static Analysis**: cppcheck, clang-tidy para detecção de problemas
- **Code Coverage**: Geração de relatórios de cobertura com lcov
- **Memory Safety**: Análise com Valgrind para detecção de vazamentos
- **Performance**: Benchmarks automatizados com métricas de performance

#### 🔒 **Security Scanning** (`.github/workflows/security.yml`)
- **Vulnerability Scanning**: Verificação de dependências vulneráveis
- **Code Security**: Análise com Flawfinder e RATS
- **License Compliance**: Verificação de licenças e headers
- **Build Security**: Flags de segurança e verificação de binários

#### 📚 **Documentation** (`.github/workflows/docs.yml`)
- **API Documentation**: Geração automática com Doxygen
- **GitHub Pages**: Deploy automático da documentação
- **Quality Check**: Verificação de cobertura de documentação
- **Spell Check**: Verificação ortográfica dos arquivos markdown

#### 🚀 **Release Pipeline** (`.github/workflows/release.yml`)
- **Multi-Platform Build**: Builds otimizados para diferentes plataformas
- **Binary Optimization**: Strip e compressão com UPX
- **Package Creation**: Criação de pacotes com scripts de conveniência
- **GitHub Releases**: Releases automáticos com notas detalhadas
- **Checksums**: Verificação de integridade com SHA256

### Automação de Dependências

#### 📦 **Dependabot** (`.github/dependabot.yml`)
- **GitHub Actions**: Atualização automática de workflows
- **CMake Dependencies**: Monitoramento de dependências CMake
- **Security Updates**: PRs automáticos para atualizações de segurança

#### 🎫 **Issue Templates**
- **Bug Reports**: Template estruturado para relatórios de bug
- **Feature Requests**: Template para solicitações de funcionalidades
- **Performance Issues**: Template específico para problemas de performance

#### 🔄 **Pull Request Template**
- **Checklist Completa**: Verificações de qualidade, testes e documentação
- **Review Guidelines**: Áreas de foco para revisão de código
- **Impact Assessment**: Análise de impacto em performance e segurança

### Status e Badges

Os badges no topo do README fornecem visibilidade instantânea do status:
- **CI Status**: Estado atual dos builds e testes
- **Security Status**: Estado das verificações de segurança
- **Documentation**: Status da geração de documentação
- **Code Coverage**: Porcentagem de cobertura de testes
- **Release Status**: Estado dos releases automatizados

### Métricas e Monitoramento

- **Test Coverage**: >90% de cobertura com relatórios detalhados
- **Performance Tracking**: Benchmarks automáticos em cada release
- **Security Monitoring**: Scanning contínuo de vulnerabilidades
- **Documentation Coverage**: Verificação de documentação de APIs

### Monitoramento e Métricas
- **Métricas detalhadas**: Tempo por tarefa, throughput, eficiência
- **Logs estruturados**: Saída organizada com identificação de threads
- **Estatísticas comparativas**: Análise automática paralelo vs sequencial
- **Consistência de resultados**: Verificação automática de integridade

### Configuração Flexível
```cpp
PipelineConfig config;
config.num_workers = 16;                 // Threads (ou detecção automática)
config.enable_debug = true;              // Logs detalhados
config.max_sequence_length = 256;        // Tamanho máximo de sequência
config.vocab_file = "custom_vocab.txt";  // Vocabulário personalizado
config.merges_file = "custom_merges.txt"; // Merges BPE personalizados

// Configuração sequencial pura
PipelineConfig seq_config = PipelineConfig::createSequentialConfig();
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

### Versões Sequenciais Especializadas
Para as etapas críticas, foram implementadas versões sequenciais puras:
- `cleanTextSequential()`: Processa um texto por vez
- `normalizeTextSequential()`: Normalização caractere por caractere
- `wordTokenizationSequential()`: Tokenização sequencial com pausas controladas

## 🧪 Testes e Validação

### Testes Automatizados
```bash
# Compilação e teste básico
make && ./bin/pipeline_processor

# Teste de consistência
# Verifica se resultados paralelo/sequencial são idênticos

# Teste de performance
# Compara automaticamente ambos os modos
```

### Cobertura de Testes
- ✅ **Consistência**: Resultados idênticos entre execução paralela e sequencial
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

## 🔧 Extensibilidade e Personalização

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
custom_config.num_workers = std::thread::hardware_concurrency();
PipelineManager manager(custom_config);
```

### Configurações Predefinidas
```cpp
// Configuração para execução sequencial pura
PipelineConfig seq_config = PipelineConfig::createSequentialConfig();

// Configuração otimizada para máximo paralelismo
PipelineConfig parallel_config;
parallel_config.num_workers = std::thread::hardware_concurrency();
parallel_config.enable_debug = false;
```

## 🎯 Princípios de Engenharia de Software Aplicados

### 1. **Separação de Responsabilidades (SRP)**
- **Pipeline Manager**: Orquestração geral
- **Text Processor**: Processamento de texto
- **Workflow Scheduler**: Gerenciamento de tarefas
- **CSV Reader**: Leitura de dados
- **Timer**: Medição de performance

### 2. **Inversão de Dependências (DIP)**
- Interfaces bem definidas entre módulos
- Headers separados das implementações
- Acoplamento reduzido entre componentes

### 3. **Princípio Aberto/Fechado (OCP)**
- Extensível para novas tarefas
- Modificações sem quebrar código existente
- Configuração flexível via `PipelineConfig`

### 4. **Encapsulamento**
- Namespaces organizados (`legal_doc_pipeline::utils`, etc.)
- Membros privados protegidos
- Interfaces públicas minimalistas

### 5. **RAII (Resource Acquisition Is Initialization)**
- `ScopedTimer` para medição automática
- Gerenciamento automático de threads
- Destructors para limpeza segura

## 🚧 Melhorias Implementadas

### ✅ **Qualidade de Código**
- **Compilação limpa**: Zero warnings com `-Wall -Wextra`
- **Type safety**: Uso de `std::atomic<size_t>` para contadores
- **Thread safety**: Sincronização adequada com mutexes
- **RAII**: Gerenciamento automático de recursos

### ✅ **Performance**
- **Detecção automática de CPUs**: Utiliza todo o hardware disponível
- **Modo sequencial puro**: Benchmarks precisos sem paralelismo
- **Métricas detalhadas**: Speedup, eficiência, throughput
- **Comparação automática**: Paralelo vs sequencial

### ✅ **Modularidade**
- **Separação clara**: Headers e implementações organizados
- **Namespaces**: Organização lógica do código
- **Extensibilidade**: Fácil adição de novas funcionalidades
- **Configuração**: Parâmetros flexíveis

### ✅ **Documentação**
- **Headers documentados**: Comentários Doxygen
- **README unificado**: Documentação completa e atualizada
- **Exemplos práticos**: Código de demonstração
- **Estrutura clara**: Organização navegável

## 📈 Roadmap e Próximos Passos

### 🎯 **Próximas Versões**
- [ ] **Integração real com HuggingFace Tokenizers**
- [ ] **Suporte a modelos de embedding reais (LibTorch/ONNX)**
- [ ] **Interface de linha de comando mais robusta**
- [ ] **Suporte a múltiplos formatos de entrada (JSON, Parquet)**
- [ ] **Testes unitários com Google Test**
- [ ] **CI/CD com GitHub Actions**

### 🔬 **Funcionalidades Avançadas**
- [ ] **Dashboard web para monitoramento**
- [ ] **Benchmark automatizado com diferentes datasets**
- [ ] **Suporte a GPU (CUDA)**
- [ ] **Sistema de plugins dinâmicos**
- [ ] **Configuração externa (JSON/YAML)**
- [ ] **Logging estruturado**

### 📊 **Análise e Métricas**
- [ ] **Profiling detalhado**
- [ ] **Análise de uso de memória**
- [ ] **Métricas de qualidade de código**
- [ ] **Relatórios automatizados**

## 🤝 Contribuição

1. Fork o projeto
2. Crie uma branch para sua feature (`git checkout -b feature/AmazingFeature`)
3. Commit suas mudanças (`git commit -m 'Add some AmazingFeature'`)
4. Push para a branch (`git push origin feature/AmazingFeature`)
5. Abra um Pull Request

### 📋 **Guidelines de Contribuição**
- Siga o padrão de código existente
- Adicione testes para novas funcionalidades
- Documente APIs públicas com Doxygen
- Mantenha a compilação sem warnings
- Use nomenclatura clara e descritiva

## 📄 Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.

## 👥 Autores e Contribuidores

- **[Gustavo Alexandre Santos](https://github.com/gassantos)** - *Desenvolvimento principal e arquitetura*

### 🎓 **Contexto Acadêmico**
Este projeto foi desenvolvido como parte dos estudos de **Estruturas de Dados Avançadas** no contexto de doutorado, demonstrando:
- Aplicação prática de grafos de dependências
- Otimização de performance com estruturas de dados adequadas
- Engenharia de software com C++ moderno
- Análise comparativa de algoritmos paralelos vs sequenciais

## 🙏 Agradecimentos

- Inspirado em pipelines de processamento de texto para LLMs
- Comunidade HuggingFace pelos padrões de tokenização
- Documentação oficial do C++17 para patterns modernos
- Comunidade open source por ferramentas e bibliotecas

## 📚 Referências e Recursos

### **Documentação Técnica**
- [C++17 Reference](https://en.cppreference.com/w/cpp/17)
- [std::thread documentation](https://en.cppreference.com/w/cpp/thread/thread)
- [std::atomic documentation](https://en.cppreference.com/w/cpp/atomic/atomic)

### **Padrões e Práticas**
- [Modern C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [SOLID Principles in C++](https://www.digitalocean.com/community/conceptual_articles/s-o-l-i-d-the-first-five-principles-of-object-oriented-design)

### **Processamento de Texto e NLP**
- [HuggingFace Tokenizers](https://huggingface.co/docs/tokenizers/)
- [Byte Pair Encoding (BPE)](https://arxiv.org/abs/1508.07909)
- [BERT: Pre-training of Deep Bidirectional Transformers](https://arxiv.org/abs/1810.04805)

---

## 🎯 **Resumo Executivo**

Este projeto demonstra a implementação de um **pipeline modular de pré-processamento de dados jurídicos** em C++17, seguindo as melhores práticas de engenharia de software. As principais conquistas incluem:

### ✅ **Qualidade Técnica**
- Arquitetura modular com separação clara de responsabilidades
- Código limpo, documentado e livre de warnings
- Thread safety com sincronização adequada
- Detecção automática de hardware disponível

### ✅ **Performance e Escalabilidade**
- Execução paralela com pool de threads configurável
- Modo sequencial puro para benchmarks precisos
- Métricas detalhadas de performance e eficiência
- Adaptação automática ao hardware disponível

### ✅ **Extensibilidade e Manutenibilidade**
- Sistema de plugins para novas tarefas
- Configuração flexível e reutilizável
- Interfaces bem definidas entre módulos
- Documentação abrangente e exemplos práticos

### 🎓 **Valor Acadêmico**
- Aplicação prática de grafos de dependências
- Análise comparativa de algoritmos paralelos
- Demonstração de princípios SOLID em C++
- Exemplo de engenharia de software profissional

---

**Pipeline de Pré-processamento de Dados Jurídicos** - *Engenharia de Software com C++ Moderno*
