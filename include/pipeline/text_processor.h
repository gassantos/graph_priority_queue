#ifndef PIPELINE_TEXT_PROCESSOR_H
#define PIPELINE_TEXT_PROCESSOR_H

#include <vector>
#include <string>
#include <map>

/**
 * @file text_processor.h
 * @brief Módulo de processamento de texto
 * 
 * Contém todas as funções de pré-processamento de texto utilizadas
 * no pipeline de documentos jurídicos.
 */

namespace legal_doc_pipeline {
namespace pipeline {

    /**
     * @brief Classe responsável pelo processamento de texto
     */
    class TextProcessor {
    private:
        static std::map<std::string, int> vocabulary;           ///< Vocabulário para conversão de tokens
        static bool vocabulary_initialized;                     ///< Flag de inicialização do vocabulário
        static const int UNK_TOKEN_ID = 0;                     ///< ID para tokens desconhecidos

        /**
         * @brief Inicializa o vocabulário se não foi inicializado
         */
        static void initializeVocabulary();

    public:
        /**
         * @brief Construtor padrão
         */
        TextProcessor() = default;

        /**
         * @brief Destrutor
         */
        ~TextProcessor() = default;

        /**
         * @brief Limpa texto de forma puramente sequencial (sem otimizações do compilador)
         * @param texts Vetor de textos a serem limpos
         */
        static void cleanTextSequential(std::vector<std::string>& texts);

        /**
         * @brief Normaliza texto de forma puramente sequencial
         * @param texts Vetor de textos a serem normalizados
         */
        static void normalizeTextSequential(std::vector<std::string>& texts);

        /**
         * @brief Tokenização de palavras de forma puramente sequencial
         * @param texts Vetor de textos a serem tokenizados
         */
        static void wordTokenizationSequential(std::vector<std::string>& texts);

        /**
         * @brief Realiza limpeza do texto (remove HTML, caracteres especiais, etc.)
         * @param texts Vetor de textos a serem limpos
         */
        static void cleanText(std::vector<std::string>& texts);

        /**
         * @brief Normaliza o texto (converte para minúsculas, etc.)
         * @param texts Vetor de textos a serem normalizados
         */
        static void normalizeText(std::vector<std::string>& texts);

        /**
         * @brief Realiza tokenização por palavras
         * @param texts Vetor de textos a serem tokenizados
         */
        static void wordTokenization(std::vector<std::string>& texts);

        /**
         * @brief Realiza tokenização BPE (Byte Pair Encoding)
         * @param texts Vetor de textos a serem tokenizados
         */
        static void bpeTokenization(std::vector<std::string>& texts);

        /**
         * @brief Particiona tokens em sequências de tamanho limitado
         * @param texts Vetor de textos tokenizados
         * @param max_length Tamanho máximo da sequência (padrão: 128)
         */
        static void partitionTokens(std::vector<std::string>& texts, size_t max_length = 128);

        /**
         * @brief Adiciona tokens especiais ([CLS], [SEP], [EOF])
         * @param texts Vetor de textos tokenizados
         */
        static void addSpecialTokens(std::vector<std::string>& texts);

        /**
         * @brief Converte tokens para índices numéricos
         * @param texts Vetor de textos tokenizados
         */
        static void tokensToIndices(std::vector<std::string>& texts);

        /**
         * @brief Gera embeddings simulados
         * @param texts Vetor de textos com índices
         */
        static void generateEmbeddings(std::vector<std::string>& texts);

        /**
         * @brief Obtém estatísticas do vocabulário
         * @return Mapa com estatísticas (tamanho, tokens mais comuns, etc.)
         */
        static std::map<std::string, size_t> getVocabularyStats();

        /**
         * @brief Define um vocabulário customizado
         * @param custom_vocab Vocabulário customizado
         */
        static void setCustomVocabulary(const std::map<std::string, int>& custom_vocab);

        /**
         * @brief Limpa e reinicializa o vocabulário
         */
        static void resetVocabulary();
    };

} // namespace pipeline
} // namespace legal_doc_pipeline

#endif // PIPELINE_TEXT_PROCESSOR_H
