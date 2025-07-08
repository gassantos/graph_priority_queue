#include "../../include/pipeline/text_processor.h"
#include "../../include/tokenizer/tokenizer_wrapper.h"
#include <algorithm>
#include <regex>
#include <iostream>
#include <sstream>
#include <numeric>
#include <thread>
#include <chrono>

namespace legal_doc_pipeline {
namespace pipeline {

    // Inicialização das variáveis estáticas
    std::map<std::string, int> TextProcessor::vocabulary;
    bool TextProcessor::vocabulary_initialized = false;
    const int TextProcessor::UNK_TOKEN_ID;

    void TextProcessor::initializeVocabulary() {
        if (vocabulary_initialized) return;
        
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
        
        vocabulary_initialized = true;
    }

    void TextProcessor::cleanText(std::vector<std::string>& texts) {
        std::cout << "  [Task] Executando CleanText..." << std::endl;
        
        for (std::string& text : texts) {
            // Remove tags HTML
            text = std::regex_replace(text, std::regex("<.*?>"), " ");
            
            // Decodifica entidades HTML comuns
            text = std::regex_replace(text, std::regex("&amp;"), "&");
            text = std::regex_replace(text, std::regex("&lt;"), "<");
            text = std::regex_replace(text, std::regex("&gt;"), ">");
            text = std::regex_replace(text, std::regex("&quot;"), "\"");
            text = std::regex_replace(text, std::regex("&apos;"), "'");
            text = std::regex_replace(text, std::regex("&nbsp;"), " ");
            
            // Mantém apenas caracteres alfanuméricos, acentuados e espaços
            text = std::regex_replace(text, 
                std::regex("[^a-zA-Z0-9\\sÀ-ÿ]", std::regex::ECMAScript | std::regex::collate), 
                " ");
            
            // Substitui múltiplos espaços por um único espaço
            text = std::regex_replace(text, std::regex("\\s+"), " ");
            
            // Remove espaços no início e fim
            text = std::regex_replace(text, std::regex("^\\s+|\\s+$"), "");
        }
        
        std::cout << "  [Task] CleanText concluído." << std::endl;
    }

    void TextProcessor::normalizeText(std::vector<std::string>& texts) {
        std::cout << "  [Task] Executando NormalizeText..." << std::endl;
        
        for (std::string& text : texts) {
            std::transform(text.begin(), text.end(), text.begin(),
                          [](unsigned char c){ return std::tolower(c); });
        }
        
        std::cout << "  [Task] NormalizeText concluído." << std::endl;
    }

    void TextProcessor::wordTokenization(std::vector<std::string>& texts) {
        std::cout << "  [Task] Executando WordTokenization (aprimorado)..." << std::endl;
        
        // Expressão regular para encontrar palavras e pontuação
        std::regex word_punct_regex("[a-zA-Z0-9À-ÿ]+|[.,!?;:\"'()\\[\\]{}]", 
                                   std::regex::ECMAScript | std::regex::collate);

        for (std::string& text : texts) {
            std::vector<std::string> tokens;
            auto words_begin = std::sregex_iterator(text.begin(), text.end(), word_punct_regex);
            auto words_end = std::sregex_iterator();

            for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                std::smatch match = *i;
                std::string token = match.str();
                
                // Filtra tokens vazios ou de apenas espaço
                if (!token.empty() && !std::all_of(token.begin(), token.end(), ::isspace)) {
                    tokens.push_back(token);
                }
            }
            
            // Reconstrói o texto como uma string de tokens separados por espaço
            text = std::accumulate(tokens.begin(), tokens.end(), std::string(),
                                  [](const std::string& a, const std::string& b) {
                                      return a.empty() ? b : a + " " + b;
                                  });
        }
        
        std::cout << "  [Task] WordTokenization concluído." << std::endl;
    }

    void TextProcessor::bpeTokenization(std::vector<std::string>& texts) {
        std::cout << "  [Task] Executando BPETokenization..." << std::endl;
        
        try {
            TokenizerWrapper tokenizer("vocab.txt", "merges.txt");
            
            for (std::string& text : texts) {
                // Simula a tokenização BPE
                auto encoding = tokenizer.tokenize_and_add_special_tokens(text);
                
                // Converte de volta para string para manter compatibilidade
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

    void TextProcessor::partitionTokens(std::vector<std::string>& texts, size_t max_length) {
        std::cout << "  [Task] Executando PartitionTokens..." << std::endl;
        
        std::vector<std::string> partitioned_texts;

        for (const std::string& text_tokens_str : texts) {
            std::istringstream iss(text_tokens_str);
            std::string token;
            std::vector<std::string> tokens;
            
            // Extrai todos os tokens da string
            while (iss >> token) {
                tokens.push_back(token);
            }

            if (tokens.size() > max_length) {
                // Trunca a sequência para o tamanho máximo
                std::vector<std::string> truncated_tokens(tokens.begin(), tokens.begin() + max_length);
                
                // Reconstrói a string truncada
                std::string truncated_str = "";
                for (size_t i = 0; i < truncated_tokens.size(); ++i) {
                    truncated_str += truncated_tokens[i];
                    if (i < truncated_tokens.size() - 1) {
                        truncated_str += " ";
                    }
                }
                partitioned_texts.push_back(truncated_str);
            } else {
                partitioned_texts.push_back(text_tokens_str);
            }
        }
        
        texts = partitioned_texts;
        std::cout << "  [Task] PartitionTokens concluído." << std::endl;
    }

    void TextProcessor::addSpecialTokens(std::vector<std::string>& texts) {
        std::cout << "  [Task] Executando AddSpecialTokens..." << std::endl;
        
        for (std::string& text : texts) {
            // Adiciona [EOF] no final se não estiver presente
            if (text.find("[EOF]") == std::string::npos) {
                text += " [EOF]";
            }
            
            // Garante que há [CLS] no início se não estiver presente
            if (text.find("[CLS]") != 0) {
                text = "[CLS] " + text;
            }
            
            // Garante que há [SEP] antes do [EOF] se não estiver presente
            if (text.find("[SEP]") == std::string::npos) {
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

    void TextProcessor::tokensToIndices(std::vector<std::string>& texts) {
        std::cout << "[Task] Executando TokensToIndices (simulado)..." << std::endl;
        
        // Assegura que o vocabulário esteja inicializado
        initializeVocabulary();

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
                    final_indexed_sequence.push_back(UNK_TOKEN_ID);
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

    void TextProcessor::generateEmbeddings(std::vector<std::string>& texts) {
        std::cout << "[Task] Executando GenerateEmbeddings (simulado - gerando placeholders de embeddings)..." << std::endl;
        
        // Em uma implementação real, receberia os IDs numéricos e passaria por um modelo
        for (size_t i = 0; i < texts.size(); ++i) {
            texts[i] = "EMBEDDED_DOCUMENT_" + std::to_string(i + 1);
        }
        
        std::cout << "[Task] GenerateEmbeddings concluído." << std::endl;
    }

    std::map<std::string, size_t> TextProcessor::getVocabularyStats() {
        initializeVocabulary();
        
        std::map<std::string, size_t> stats;
        stats["vocabulary_size"] = vocabulary.size();
        stats["special_tokens"] = 4; // [CLS], [SEP], [EOF], [UNK]
        stats["legal_tokens"] = vocabulary.size() - 4;
        
        return stats;
    }

    void TextProcessor::setCustomVocabulary(const std::map<std::string, int>& custom_vocab) {
        vocabulary = custom_vocab;
        vocabulary_initialized = true;
    }

    void TextProcessor::resetVocabulary() {
        vocabulary.clear();
        vocabulary_initialized = false;
    }

    void TextProcessor::cleanTextSequential(std::vector<std::string>& texts) {
        std::cout << "  [Task] Executando CleanText (Sequencial Puro)..." << std::endl;
        
        // Processa um texto por vez, sem possibilidade de paralelização
        for (size_t i = 0; i < texts.size(); ++i) {
            std::string& text = texts[i];
            
            // Remove tags HTML
            text = std::regex_replace(text, std::regex("<.*?>"), " ");
            
            // Decodifica entidades HTML comuns
            text = std::regex_replace(text, std::regex("&amp;"), "&");
            text = std::regex_replace(text, std::regex("&lt;"), "<");
            text = std::regex_replace(text, std::regex("&gt;"), ">");
            text = std::regex_replace(text, std::regex("&quot;"), "\"");
            text = std::regex_replace(text, std::regex("&apos;"), "'");
            text = std::regex_replace(text, std::regex("&nbsp;"), " ");
            
            // Mantém apenas caracteres alfanuméricos, acentuados e espaços
            text = std::regex_replace(text, 
                std::regex("[^a-zA-Z0-9\\sÀ-ÿ]", std::regex::ECMAScript | std::regex::collate), 
                " ");
            
            // Substitui múltiplos espaços por um único espaço
            text = std::regex_replace(text, std::regex("\\s+"), " ");
            
            // Remove espaços no início e fim
            text = std::regex_replace(text, std::regex("^\\s+|\\s+$"), "");
            
            // Adiciona uma pequena pausa para garantir que não há otimização agressiva
            if (i % 10000 == 0 && i > 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
        
        std::cout << "  [Task] CleanText (Sequencial Puro) concluído." << std::endl;
    }

    void TextProcessor::normalizeTextSequential(std::vector<std::string>& texts) {
        std::cout << "  [Task] Executando NormalizeText (Sequencial Puro)..." << std::endl;
        
        // Processa um texto por vez
        for (size_t i = 0; i < texts.size(); ++i) {
            std::string& text = texts[i];
            
            // Converte para minúsculas caractere por caractere
            for (char& c : text) {
                c = std::tolower(static_cast<unsigned char>(c));
            }
            
            // Pequena pausa ocasional
            if (i % 10000 == 0 && i > 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
        
        std::cout << "  [Task] NormalizeText (Sequencial Puro) concluído." << std::endl;
    }

    void TextProcessor::wordTokenizationSequential(std::vector<std::string>& texts) {
        std::cout << "  [Task] Executando WordTokenization (Sequencial Puro)..." << std::endl;
        
        // Expressão regular para encontrar palavras e pontuação
        std::regex word_punct_regex("[a-zA-Z0-9À-ÿ]+|[.,!?;:\"'()\\[\\]{}]", 
                                   std::regex::ECMAScript | std::regex::collate);

        // Processa um texto por vez
        for (size_t i = 0; i < texts.size(); ++i) {
            std::string& text = texts[i];
            std::vector<std::string> tokens;
            auto words_begin = std::sregex_iterator(text.begin(), text.end(), word_punct_regex);
            auto words_end = std::sregex_iterator();

            for (std::sregex_iterator iter = words_begin; iter != words_end; ++iter) {
                std::smatch match = *iter;
                std::string token = match.str();
                
                // Filtra tokens vazios ou de apenas espaço
                if (!token.empty() && !std::all_of(token.begin(), token.end(), ::isspace)) {
                    tokens.push_back(token);
                }
            }
            
            // Reconstrói o texto como uma string de tokens separados por espaço
            text = std::accumulate(tokens.begin(), tokens.end(), std::string(),
                                  [](const std::string& a, const std::string& b) {
                                      return a.empty() ? b : a + " " + b;
                                  });
            
            // Pequena pausa ocasional
            if (i % 10000 == 0 && i > 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
        
        std::cout << "  [Task] WordTokenization (Sequencial Puro) concluído." << std::endl;
    }
} // namespace pipeline
} // namespace legal_doc_pipeline
