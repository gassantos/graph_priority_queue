#include <gtest/gtest.h>
#include "../include/utils/csv_reader.h"
#include <fstream>
#include <filesystem>

/**
 * @file test_csv_reader.cpp
 * @brief Testes unitários para a classe CsvReader
 */

namespace fs = std::filesystem;
using namespace legal_doc_pipeline::utils;

class CsvReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Criar arquivo CSV de teste
        test_csv_content = 
            "ID,Nome,Texto,Categoria\n"
            "1,\"Doc1\",\"Este é um texto de teste\",Jurídico\n"
            "2,\"Doc2\",\"Segundo documento para teste\",Administrativo\n"
            "3,\"Doc3\",\"Terceiro texto com vírgulas, aspas e quebras\",Legal\n"
            "4,\"Doc4\",\"Texto com caracteres especiais: àáâãäåçèéê\",Especial\n";
        
        std::ofstream file(test_filename);
        file << test_csv_content;
        file.close();
        
        // Criar arquivo CSV malformado
        malformed_csv_content = 
            "ID,Nome,Texto\n"
            "1,\"Doc sem fechamento de aspas,teste\n"
            "2,Doc2,Normal\n";
        
        std::ofstream malformed_file(malformed_filename);
        malformed_file << malformed_csv_content;
        malformed_file.close();
    }
    
    void TearDown() override {
        // Limpar arquivos de teste
        if (fs::exists(test_filename)) {
            fs::remove(test_filename);
        }
        if (fs::exists(malformed_filename)) {
            fs::remove(malformed_filename);
        }
    }
    
    const std::string test_filename = "tests/test_data.csv";
    const std::string malformed_filename = "tests/malformed_test.csv";
    const std::string nonexistent_filename = "tests/nonexistent_file.csv";
    std::string test_csv_content;
    std::string malformed_csv_content;
    CsvReader reader;
};

// Teste de validação de arquivo existente
TEST_F(CsvReaderTest, ValidateFileExists) {
    EXPECT_TRUE(reader.validateFile(test_filename));
}

// Teste de validação de arquivo inexistente
TEST_F(CsvReaderTest, ValidateFileNotExists) {
    EXPECT_FALSE(reader.validateFile(nonexistent_filename));
}

// Teste de leitura de coluna válida
TEST_F(CsvReaderTest, ReadValidColumn) {
    auto result = reader.readColumn(test_filename, "Texto", ',');
    
    EXPECT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], "Este é um texto de teste");
    EXPECT_EQ(result[1], "Segundo documento para teste");
    EXPECT_EQ(result[2], "Terceiro texto com vírgulas, aspas e quebras");
    EXPECT_EQ(result[3], "Texto com caracteres especiais: àáâãäåçèéê");
}

// Teste de leitura de coluna inexistente
TEST_F(CsvReaderTest, ReadInvalidColumn) {
    auto result = reader.readColumn(test_filename, "ColunaInexistente", ',');
    EXPECT_TRUE(result.empty());
}

// Teste de leitura de diferentes colunas
TEST_F(CsvReaderTest, ReadDifferentColumns) {
    auto ids = reader.readColumn(test_filename, "ID", ',');
    auto nomes = reader.readColumn(test_filename, "Nome", ',');
    auto categorias = reader.readColumn(test_filename, "Categoria", ',');
    
    EXPECT_EQ(ids.size(), 4);
    EXPECT_EQ(nomes.size(), 4);
    EXPECT_EQ(categorias.size(), 4);
    
    EXPECT_EQ(ids[0], "1");
    EXPECT_EQ(nomes[0], "Doc1");
    EXPECT_EQ(categorias[0], "Jurídico");
}

// Teste de robustez com arquivo malformado
TEST_F(CsvReaderTest, HandleMalformedCSV) {
    // Deve ainda conseguir ler algumas linhas válidas
    auto result = reader.readColumn(malformed_filename, "Nome", ',');
    EXPECT_GE(result.size(), 1); // Pelo menos uma linha válida
}

// Teste de performance com arquivo vazio
TEST_F(CsvReaderTest, EmptyFile) {
    std::string empty_filename = "empty_test.csv";
    std::ofstream empty_file(empty_filename);
    empty_file << "";
    empty_file.close();
    
    auto result = reader.readColumn(empty_filename, "Texto", ',');
    EXPECT_TRUE(result.empty());
    
    fs::remove(empty_filename);
}

// Teste de arquivo apenas com cabeçalho
TEST_F(CsvReaderTest, HeaderOnlyFile) {
    std::string header_only_filename = "header_only_test.csv";
    std::ofstream header_file(header_only_filename);
    header_file << "ID,Nome,Texto\n";
    header_file.close();
    
    auto result = reader.readColumn(header_only_filename, "Texto", ',');
    EXPECT_TRUE(result.empty());
    
    fs::remove(header_only_filename);
}

// Teste de case sensitivity no nome da coluna
TEST_F(CsvReaderTest, ColumnNameCaseSensitive) {
    auto result_lower = reader.readColumn(test_filename, "texto", ',');
    auto result_upper = reader.readColumn(test_filename, "TEXTO", ',');
    auto result_correct = reader.readColumn(test_filename, "Texto", ',');
    
    EXPECT_TRUE(result_lower.empty());
    EXPECT_TRUE(result_upper.empty());
    EXPECT_FALSE(result_correct.empty());
}

// Teste de validação com arquivo sem permissão de leitura
TEST_F(CsvReaderTest, FilePermissions) {
    // Este teste pode ser específico do sistema
    // Em sistemas Unix, podemos testar permissões
    #ifdef __unix__
    std::string no_perm_filename = "no_permission_test.csv";
    std::ofstream file(no_perm_filename);
    file << test_csv_content;
    file.close();
    
    // Remover permissão de leitura
    fs::permissions(no_perm_filename, fs::perms::none);
    
    EXPECT_FALSE(reader.validateFile(no_perm_filename));
    
    // Restaurar permissões para limpeza
    fs::permissions(no_perm_filename, fs::perms::owner_all);
    fs::remove(no_perm_filename);
    #endif
}
