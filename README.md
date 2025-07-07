# graph_priority_summary

Este repositório contém uma implementação em C++ de um pipeline de pré-processamento de dados textuais jurídicos. O pipeline é projetado para processar a coluna 'Texto' de um arquivo CSV, aplicando uma série de transformações. O foco principal da solução é a orquestração de tarefas utilizando um grafo de dependência e uma fila de prioridade para execução paralela _multithreaded_. Para fins de comparação de desempenho, também é incluída uma implementação sequencial do mesmo pipeline.

## 1. Descrição do Projeto

O objetivo deste projeto é demonstrar uma arquitetura de pré-processamento de dados escalável e eficiente, crucial para o treinamento e fine-tuning de Large Language Models (LLMs) em domínios específicos, como o jurídico.

### 1.1. Componentes Principais

* **Estrutura `Task`**: Representa uma unidade de trabalho no pipeline. Cada `Task` possui um ID, prioridade, lista de dependências (tarefas que devem ser concluídas antes), lista de tarefas dependentes (que aguardam a sua conclusão) e uma função (`std::function`) que encapsula a lógica de pré-processamento. A contagem de dependências restantes (`std::atomic<int>`) garante a segurança em ambientes multithread.
* **`WorkflowScheduler`**: Esta classe é o coração da orquestração paralela. Ela gerencia um mapa de todas as `Task`s, uma fila de prioridade (`std::priority_queue`) que armazena ponteiros para tarefas prontas para execução (garantindo que tarefas de maior prioridade sejam processadas primeiro), e mecanismos de sincronização (`std::mutex`, `std::condition_variable`) para coordenar threads de trabalho. Múltiplos "workers" (`std::thread`) extraem tarefas da fila de prioridade e as executam.
* **Funções de Pré-processamento**: São funções independentes que recebem o `std::vector<std::string>` contendo os dados textuais e aplicam as transformações. As etapas incluem:
    * **`readCsvColumn`**: Função utilitária para carregar a coluna desejada de um arquivo CSV.
    * **`cleanText`**: Remove tags HTML, caracteres indesejados e espaços extras.
    * **`normalizeText`**: Converte o texto para minúsculas.
    * **`wordTokenization`**: efetua a tokenização de palavras.
    * **`bpeTokenization`**: efetua a aplicação de um tokenizador BPE.
    * **`partitionTokens`**: efetua a partição de texto em sequências de tokens.
    * **`addSpecialTokens`**: Adiciona tokens especiais (e.g., `[CLS]`, `[SEP]`, `[EOF]`).
    * **`tokensToIndices`**: efetua a conversão de tokens para IDs numéricos.
    * **`generateEmbeddings`**: efetua a geração de vetores de embeddings.
* **Pipeline Sequencial (`run_sequential_pipeline`)**: Um método de comparação que executa as mesmas funções de pré-processamento em uma única thread, de forma estritamente sequencial, sem qualquer orquestração de grafo ou paralelismo.

### 1.2. Fluxo de Trabalho

O pipeline opera da seguinte forma:
1.  Os dados brutos da coluna `Texto` são lidos do arquivo `csv`.
2.  Para o **cenário paralelo**, as tarefas são definidas com suas prioridades e adicionadas a um `WorkflowScheduler`. As dependências entre as tarefas são estabelecidas (ex: `NormalizeText` depende de `CleanText`).
3.  O `WorkflowScheduler` inicializa uma fila de prioridade com as tarefas sem dependências.
4.  Um número configurável de threads de worker é iniciado. Cada worker pega uma tarefa da fila de prioridade (a de maior prioridade e com todas as dependências satisfeitas), a executa, e então sinaliza sua conclusão.
5.  Ao completar uma tarefa, o agendador verifica e adiciona tarefas dependentes à fila de prioridade se suas dependências forem resolvidas.
6.  Para o **cenário sequencial**, uma cópia independente dos dados é processada diretamente pelas funções de pré-processamento, uma após a outra.
7.  Ambos os cenários têm seus tempos de execução medidos e comparados.

## 2. Pré-requisitos

* Compilador C++17 (g++ 7.x ou superior recomendado).
* Um ambiente de desenvolvimento Linux (como o GitHub Codespaces).

## 3. Como Compilar

Certifique-se de que os arquivo `main.cpp`, `tokenizer_wrapper` e o `csv` (com a coluna `Texto`) estejam no mesmo diretório.

Abra o terminal e execute o seguinte comando:

```bash
g++ -std=c++17 -pthread -O2 main.cpp tokenizer_wrapper.cpp -o workflow
