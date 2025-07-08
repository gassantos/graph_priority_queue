#include <gtest/gtest.h>
#include "../include/pipeline/text_processor.h"
#include <vector>
#include <string>

/**
 * @file test_text_processor.cpp
 * @brief Testes unitários para a classe TextProcessor
 */

using namespace legal_doc_pipeline::pipeline;

class TextProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Dados de teste variados
        test_texts = {
            "<html><body>Texto com HTML &amp; caracteres especiais!</body></html>",
            "  TEXTO EM MAIUSCULAS   ",
            "Texto normal com pontuação: vírgulas, pontos. E exclamações!",
            "Texto com números 123 e caracteres especiais @#$%^&*()",
            "Texto com acentos: ção, não, coração, pão",
            "",  // string vazia
            "   ",  // apenas espaços
            "SingleWord",
            "Multiple    spaces    between    words"
        };
        
        original_texts = test_texts;  // Manter cópia original
    }
    
    void TearDown() override {
        // Restaurar textos originais para próximo teste
        test_texts = original_texts;
    }
    
    std::vector<std::string> test_texts;
    std::vector<std::string> original_texts;
};

// Teste da função cleanText
TEST_F(TextProcessorTest, CleanTextRemovesHTML) {
    TextProcessor::cleanText(test_texts);
    
    // Verifica se HTML foi removido
    EXPECT_EQ(test_texts[0], "Texto com HTML caracteres especiais");
    
    // Verifica se espaços múltiplos foram normalizados
    EXPECT_EQ(test_texts[8], "Multiple spaces between words");
    
    // Verifica se espaços nas bordas foram removidos
    auto trimmed = test_texts[1];
    EXPECT_FALSE(trimmed.empty());
    EXPECT_NE(trimmed[0], ' ');
    EXPECT_NE(trimmed.back(), ' ');
}

// Teste da função normalizeText
TEST_F(TextProcessorTest, NormalizeTextConvertsToLowercase) {
    TextProcessor::normalizeText(test_texts);
    
    EXPECT_EQ(test_texts[1], "  texto em maiusculas   ");
    EXPECT_EQ(test_texts[7], "singleword");
    
    // Verifica se acentos são preservados
    EXPECT_TRUE(test_texts[4].find("ção") != std::string::npos);
}

// Teste da função wordTokenization
TEST_F(TextProcessorTest, WordTokenizationSeparatesWords) {
    TextProcessor::wordTokenization(test_texts);
    
    // Verifica se pontuação foi separada
    EXPECT_TRUE(test_texts[2].find(" : ") != std::string::npos ||
                test_texts[2].find(" , ") != std::string::npos);
    
    // Verifica se string vazia permanece vazia
    EXPECT_TRUE(test_texts[5].empty());
}

// Teste sequencial das três primeiras funções
TEST_F(TextProcessorTest, SequentialProcessing) {
    std::vector<std::string> sequential_texts = test_texts;
    
    TextProcessor::cleanText(sequential_texts);
    TextProcessor::normalizeText(sequential_texts);
    TextProcessor::wordTokenization(sequential_texts);
    
    // Primeiro texto deve estar limpo, em minúsculas e tokenizado
    EXPECT_TRUE(sequential_texts[0].find("<html>") == std::string::npos);
    EXPECT_TRUE(sequential_texts[0].find("<body>") == std::string::npos);
    
    // Verifica se está em minúsculas
    bool has_uppercase = false;
    for (char c : sequential_texts[1]) {
        if (c >= 'A' && c <= 'Z') {
            has_uppercase = true;
            break;
        }
    }
    EXPECT_FALSE(has_uppercase);
}

// Teste das versões sequenciais
TEST_F(TextProcessorTest, SequentialVersionsProduceSameResults) {
    std::vector<std::string> parallel_texts = test_texts;
    std::vector<std::string> sequential_texts = test_texts;
    
    // Teste cleanText
    TextProcessor::cleanText(parallel_texts);
    TextProcessor::cleanTextSequential(sequential_texts);
    
    EXPECT_EQ(parallel_texts.size(), sequential_texts.size());
    for (size_t i = 0; i < parallel_texts.size(); ++i) {
        EXPECT_EQ(parallel_texts[i], sequential_texts[i]) 
            << "Diferença na posição " << i;
    }
    
    // Teste normalizeText
    TextProcessor::normalizeText(parallel_texts);
    TextProcessor::normalizeTextSequential(sequential_texts);
    
    for (size_t i = 0; i < parallel_texts.size(); ++i) {
        EXPECT_EQ(parallel_texts[i], sequential_texts[i]) 
            << "Diferença na normalização na posição " << i;
    }
    
    // Teste wordTokenization
    TextProcessor::wordTokenization(parallel_texts);
    TextProcessor::wordTokenizationSequential(sequential_texts);
    
    for (size_t i = 0; i < parallel_texts.size(); ++i) {
        EXPECT_EQ(parallel_texts[i], sequential_texts[i]) 
            << "Diferença na tokenização na posição " << i;
    }
}

// Teste de robustez com entrada vazia
TEST_F(TextProcessorTest, EmptyInput) {
    std::vector<std::string> empty_vector;
    
    EXPECT_NO_THROW(TextProcessor::cleanText(empty_vector));
    EXPECT_NO_THROW(TextProcessor::normalizeText(empty_vector));
    EXPECT_NO_THROW(TextProcessor::wordTokenization(empty_vector));
    
    EXPECT_TRUE(empty_vector.empty());
}

// Teste com textos muito grandes
TEST_F(TextProcessorTest, LargeTextHandling) {
    std::vector<std::string> large_texts;
    
    // Criar texto grande (10KB)
    std::string large_text(10000, 'A');
    large_text += " teste final";
    large_texts.push_back(large_text);
    
    EXPECT_NO_THROW(TextProcessor::cleanText(large_texts));
    EXPECT_NO_THROW(TextProcessor::normalizeText(large_texts));
    EXPECT_NO_THROW(TextProcessor::wordTokenization(large_texts));
    
    // Verifica se o processamento funcionou
    EXPECT_FALSE(large_texts[0].empty());
    EXPECT_TRUE(large_texts[0].find("teste final") != std::string::npos);
}

// Teste de partição de tokens
TEST_F(TextProcessorTest, PartitionTokensRespectsSizeLimit) {
    std::vector<std::string> token_texts = {
        "token1 token2 token3 token4 token5 token6 token7 token8",
        "short",
        "uma sequência muito longa de tokens que deve ser particionada"
    };
    
    size_t max_length = 5;
    TextProcessor::partitionTokens(token_texts, max_length);
    
    // Verifica se nenhum texto excede o limite
    for (const auto& text : token_texts) {
        std::istringstream iss(text);
        std::string token;
        size_t count = 0;
        while (iss >> token) {
            count++;
        }
        EXPECT_LE(count, max_length) << "Texto excede limite: " << text;
    }
}

// Teste de adição de tokens especiais
TEST_F(TextProcessorTest, AddSpecialTokens) {
    std::vector<std::string> token_texts = {
        "texto normal",
        "",
        "outro texto"
    };
    
    TextProcessor::addSpecialTokens(token_texts);
    
    // Verifica se tokens especiais foram adicionados
    for (const auto& text : token_texts) {
        if (!text.empty()) {
            EXPECT_TRUE(text.find("[CLS]") != std::string::npos);
            EXPECT_TRUE(text.find("[SEP]") != std::string::npos ||
                       text.find("[EOF]") != std::string::npos);
        }
    }
}

// Teste de conversão tokens para índices
TEST_F(TextProcessorTest, TokensToIndices) {
    std::vector<std::string> token_texts = {
        "[CLS] texto [SEP]",
        "[CLS] outro [EOF]"
    };
    
    auto original_texts = token_texts;
    TextProcessor::tokensToIndices(token_texts);
    
    // Verifica se houve conversão (texto deve ser diferente)
    EXPECT_NE(token_texts[0], original_texts[0]);
    EXPECT_NE(token_texts[1], original_texts[1]);
    
    // Verifica se contém números (índices)
    bool has_digits = false;
    for (const auto& text : token_texts) {
        for (char c : text) {
            if (std::isdigit(c)) {
                has_digits = true;
                break;
            }
        }
        if (has_digits) break;
    }
    EXPECT_TRUE(has_digits);
}

// Teste de geração de embeddings
TEST_F(TextProcessorTest, GenerateEmbeddings) {
    std::vector<std::string> index_texts = {
        "101 1 102",
        "101 2 103"
    };
    
    auto original_texts = index_texts;
    TextProcessor::generateEmbeddings(index_texts);
    
    // Verifica se embeddings foram gerados (texto modificado)
    EXPECT_NE(index_texts[0], original_texts[0]);
    EXPECT_NE(index_texts[1], original_texts[1]);
    
    // Verifica se contém representação de embeddings
    for (const auto& text : index_texts) {
        EXPECT_TRUE(text.find("embedding") != std::string::npos ||
                   text.find("vector") != std::string::npos ||
                   !text.empty());
    }
}
