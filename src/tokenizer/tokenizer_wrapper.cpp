#include "../../include/tokenizer/tokenizer_wrapper.h"

// Implementações do namespace hf_tokenizers
namespace hf_tokenizers {
    // Implementação do método add_special_tokens
    void Encoding::add_special_tokens() {
        // Em um tokenizador real, isso seria configurado no construtor ou via método dedicado.
        // Aqui, apenas adicionamos manualmente para ilustrar.
        // CLS token (id 101), SEP token (id 102), EOF (não é padrão, mas para o exemplo id 103)
        tokens.insert(tokens.begin(), {"[CLS]", 101, {}});
        ids.insert(ids.begin(), 101);
        type_ids.insert(type_ids.begin(), 0); // Exemplo de type_id

        tokens.push_back({"[SEP]", 102, {}});
        ids.push_back(102);
        type_ids.push_back(0); // Exemplo de type_id

        // EOF não é padrão em muitos modelos, mas mantido para o exemplo.
        tokens.push_back({"[EOF]", 103, {}});
        ids.push_back(103);
        type_ids.push_back(0); // Exemplo de type_id
    }

    // Implementação do método get_ids
    std::vector<unsigned int> Encoding::get_ids() const {
        return ids;
    }

    // Implementação do método len
    size_t Encoding::len() const {
        return ids.size();
    }

    // Implementação do construtor do Tokenizer
    Tokenizer::Tokenizer(const std::string& vocab_path, const std::string& merges_path) {
        std::cout << "  [TokenizerWrapper] Carregando o modelo de tokenizador BPE de: " << vocab_path << " e " << merges_path << std::endl;
        // Em uma implementação real, carregaria os arquivos do modelo SentencePiece ou BPE
        // Exemplo: tokenizer = Tokenizer::from_file(model_path);
        // Simulação de vocabulário e merges para exemplo
        vocabulary = {
            {"[CLS]", 101}, {"[SEP]", 102}, {"[EOF]", 103},
            {"o", 1}, {"e", 2}, {"a", 3}, {" ", 4}, {"do", 5},
            {"um", 6}, {"documento", 7}, {"visa", 8}, {"apresentar", 9},
            {"fluxo", 10}, {"tarefas", 11}, {"para", 12}, {"sumarização", 13},
            {"texto", 14}, {"documentos", 15}, {"jurídicos", 16}
            // ... e muitos outros tokens/subtokens
        };
        std::cout << "  [TokenizerWrapper] Modelo de tokenizador carregado com sucesso (simulado)." << std::endl;
    }

    // Implementação do método encode
    Encoding Tokenizer::encode(const std::string& text) const {
        Encoding encoding;
        std::string current_token_str;
        for (char c : text) {
            if (c == ' ' || c == '.' || c == ',' || c == '\n') { // Simples separador
                if (!current_token_str.empty()) {
                    process_token(current_token_str, encoding);
                    current_token_str.clear();
                }
                if (c == ' ') { // Adiciona espaço como token se relevante
                    process_token(" ", encoding);
                }
            } else {
                current_token_str += c;
            }
        }
        if (!current_token_str.empty()) {
            process_token(current_token_str, encoding);
        }
        return encoding;
    }

    // Implementação do método process_token
    void Tokenizer::process_token(const std::string& token_str, Encoding& encoding) const {
        // Simula o BPE: para cada "palavra", tentamos dividi-la em subtokens do vocabulário
        // Esta é uma simulação muito simplificada; um BPE real é mais complexo.
        std::string temp_str = token_str;
        while (!temp_str.empty()) {
            bool found = false;
            for (int len = temp_str.length(); len > 0; --len) {
                std::string sub_token = temp_str.substr(0, len);
                if (vocabulary.count(sub_token)) {
                    encoding.tokens.push_back({sub_token, vocabulary.at(sub_token), {}});
                    encoding.ids.push_back(vocabulary.at(sub_token));
                    encoding.type_ids.push_back(0); // Exemplo
                    temp_str = temp_str.substr(len);
                    found = true;
                    break;
                }
            }
            if (!found) {
                // Se não encontrar, trata como token desconhecido (simulação)
                encoding.tokens.push_back({temp_str, 0, {}}); // ID 0 para desconhecido
                encoding.ids.push_back(0);
                encoding.type_ids.push_back(0);
                temp_str.clear();
            }
        }
    }
} // namespace hf_tokenizers

// Implementações da classe TokenizerWrapper
TokenizerWrapper::TokenizerWrapper(const std::string& vocab_path, const std::string& merges_path) {
    // Inicializa o tokenizador. Em um cenário real, trataria exceções de carregamento.
    tokenizer = new hf_tokenizers::Tokenizer(vocab_path, merges_path);
}

TokenizerWrapper::~TokenizerWrapper() {
    delete tokenizer;
}

hf_tokenizers::Encoding TokenizerWrapper::tokenize_and_add_special_tokens(const std::string& text) {
    if (!tokenizer) {
        throw std::runtime_error("Tokenizer não inicializado.");
    }
    hf_tokenizers::Encoding encoding = tokenizer->encode(text);
    encoding.add_special_tokens(); // Adiciona tokens especiais
    return encoding;
}

std::vector<unsigned int> TokenizerWrapper::text_to_ids(const std::string& text) {
    hf_tokenizers::Encoding encoding = tokenize_and_add_special_tokens(text);
    return encoding.get_ids();
}

// Implementação dos construtores/operadores de movimento
TokenizerWrapper::TokenizerWrapper(TokenizerWrapper&& other) noexcept 
    : tokenizer(other.tokenizer) {
    other.tokenizer = nullptr;
}

TokenizerWrapper& TokenizerWrapper::operator=(TokenizerWrapper&& other) noexcept {
    if (this != &other) {
        delete tokenizer;
        tokenizer = other.tokenizer;
        other.tokenizer = nullptr;
    }
    return *this;
}