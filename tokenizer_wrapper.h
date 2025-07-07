#ifndef TOKENIZER_WRAPPER_H
#define TOKENIZER_WRAPPER_H

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <numeric>
#include <map>
#include <stdexcept>
#include <algorithm>

// Mock-up do namespace ou classes da biblioteca huggingface-tokenizer-in-cxx
// Em um ambiente real, você incluiria os arquivos de cabeçalho reais e linkaria com a biblioteca.
namespace hf_tokenizers {
    /**
     * @brief Estrutura que representa um token individual
     */
    struct Token {
        std::string text;               ///< Texto do token
        unsigned int id;                ///< ID numérico do token
        std::vector<unsigned int> bytes; ///< Representação em bytes do token
    };

    /**
     * @brief Estrutura que representa o resultado da codificação de um texto
     */
    struct Encoding {
        std::vector<Token> tokens;      ///< Lista de tokens gerados
        std::vector<unsigned int> ids;  ///< Lista de IDs dos tokens
        std::vector<int> type_ids;      ///< Lista de type IDs para segmentação

        /**
         * @brief Adiciona tokens especiais ([CLS], [SEP], [EOF]) ao encoding
         */
        void add_special_tokens();

        /**
         * @brief Obtém apenas os IDs dos tokens
         * @return Vetor com os IDs dos tokens
         */
        std::vector<unsigned int> get_ids() const;

        /**
         * @brief Obtém o tamanho do encoding (número de tokens)
         * @return Número de tokens no encoding
         */
        size_t len() const;
    };

    /**
     * @brief Classe principal do tokenizador BPE (Byte Pair Encoding)
     */
    class Tokenizer {
    private:
        std::map<std::string, unsigned int> vocabulary; ///< Vocabulário do tokenizador

        /**
         * @brief Processa um token individual e o adiciona ao encoding
         * @param token_str String do token a ser processado
         * @param encoding Encoding onde o token será adicionado
         */
        void process_token(const std::string& token_str, Encoding& encoding) const;

    public:
        /**
         * @brief Construtor que carrega o modelo do tokenizador
         * @param vocab_path Caminho para o arquivo de vocabulário
         * @param merges_path Caminho para o arquivo de merges BPE
         */
        Tokenizer(const std::string& vocab_path, const std::string& merges_path);

        /**
         * @brief Realiza a tokenização de um texto
         * @param text Texto a ser tokenizado
         * @return Encoding com os tokens gerados
         */
        Encoding encode(const std::string& text) const;
    };
}

/**
 * @brief Classe wrapper para gerenciar o ciclo de vida do tokenizador
 * 
 * Esta classe encapsula o tokenizador HuggingFace e fornece métodos
 * convenientes para tokenização com gerenciamento automático de memória.
 */
class TokenizerWrapper {
private:
    hf_tokenizers::Tokenizer* tokenizer; ///< Ponteiro para o objeto do tokenizador

public:
    /**
     * @brief Construtor que carrega o tokenizador
     * @param vocab_path Caminho para o arquivo de vocabulário
     * @param merges_path Caminho para o arquivo de merges BPE
     */
    TokenizerWrapper(const std::string& vocab_path, const std::string& merges_path);

    /**
     * @brief Destrutor que libera a memória do tokenizador
     */
    ~TokenizerWrapper();

    /**
     * @brief Realiza tokenização BPE e adiciona tokens especiais
     * @param text Texto a ser tokenizado
     * @return Encoding com tokens e tokens especiais
     * @throws std::runtime_error se o tokenizador não estiver inicializado
     */
    hf_tokenizers::Encoding tokenize_and_add_special_tokens(const std::string& text);

    /**
     * @brief Converte texto diretamente para IDs de tokens
     * @param text Texto a ser convertido
     * @return Vetor com os IDs dos tokens (incluindo tokens especiais)
     */
    std::vector<unsigned int> text_to_ids(const std::string& text);

    // Impede cópia para evitar problemas com gerenciamento de memória
    TokenizerWrapper(const TokenizerWrapper&) = delete;
    TokenizerWrapper& operator=(const TokenizerWrapper&) = delete;

    // Permite movimentação se necessário no futuro
    TokenizerWrapper(TokenizerWrapper&& other) noexcept;
    TokenizerWrapper& operator=(TokenizerWrapper&& other) noexcept;
};

#endif // TOKENIZER_WRAPPER_H
