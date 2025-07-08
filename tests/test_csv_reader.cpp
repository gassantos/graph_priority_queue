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

// Testes com arquivo de dados reais do projeto
TEST_F(CsvReaderTest, ReadRealDataFile) {
    const std::string real_data_file = "tests/test_docs.csv";
    
    // Verificar se o arquivo existe
    ASSERT_TRUE(reader.validateFile(real_data_file));
    
    // Verificar se podemos ler os nomes das colunas (usando delimitador correto ';')
    auto columns = reader.getColumnNames(real_data_file, ';');
    EXPECT_FALSE(columns.empty());
    
    // Verificar colunas específicas esperadas
    bool has_processo = std::find(columns.begin(), columns.end(), "Processo") != columns.end();
    bool has_texto = std::find(columns.begin(), columns.end(), "Texto") != columns.end();
    bool has_data = std::find(columns.begin(), columns.end(), "DataSessao") != columns.end();
    
    EXPECT_TRUE(has_processo);
    EXPECT_TRUE(has_texto);
    EXPECT_TRUE(has_data);
}

TEST_F(CsvReaderTest, ReadRealDataColumn) {
    const std::string real_data_file = "tests/test_docs.csv";
    
    // Ler coluna 'Texto' dos dados reais (usando delimitador correto ';')
    auto textos = reader.readColumn(real_data_file, "Texto", ';');
    
    // Deve ter exatamente 4 entradas (5 linhas no arquivo - 1 cabeçalho)
    EXPECT_EQ(textos.size(), 4);
    
    // Verificar se o primeiro texto contém conteúdo esperado
    EXPECT_FALSE(textos[0].empty());
    EXPECT_TRUE(textos[0].find("PLENÁRIO") != std::string::npos ||
                textos[0].find("PROCESSO") != std::string::npos ||
                textos[0].find("TCE-RJ") != std::string::npos);
}

TEST_F(CsvReaderTest, ReadRealDataProcessColumn) {
    const std::string real_data_file = "tests/test_docs.csv";
    
    // Ler coluna 'Processo' dos dados reais (usando delimitador correto ';')
    auto processos = reader.readColumn(real_data_file, "Processo", ';');
    
    // Deve ter exatamente 4 entradas
    EXPECT_EQ(processos.size(), 4);
    
    // Verificar se o primeiro processo tem formato esperado (números/ano)
    EXPECT_FALSE(processos[0].empty());
    EXPECT_TRUE(processos[0].find("/2024") != std::string::npos ||
                processos[0].find("/2023") != std::string::npos ||
                processos[0].find("/2025") != std::string::npos);
}

TEST_F(CsvReaderTest, RealDataUTF8WithBOM) {
    const std::string real_data_file = "tests/test_docs.csv";
    
    // O arquivo tem BOM UTF-8, verificar se é lido corretamente
    auto columns = reader.getColumnNames(real_data_file);
    
    // A primeira coluna deve ser "Processo", não ter caracteres estranhos do BOM
    EXPECT_FALSE(columns.empty());
    EXPECT_EQ(columns[0], "Processo");
    
    // Verificar que não há caracteres de controle no início
    EXPECT_TRUE(columns[0][0] >= 'A' && columns[0][0] <= 'Z');
}

TEST_F(CsvReaderTest, RealDataAllColumns) {
    const std::string real_data_file = "tests/test_docs.csv";
    
    // Verificar todas as colunas esperadas
    auto columns = reader.getColumnNames(real_data_file);
    
    std::vector<std::string> expected_columns = {
        "Processo", "DataSessao", "Texto", "Resumo", "Legislacao",
        "Pareceres", "CorpoInstrutivo", "MinisterioPublicoContas",
        "VotoRelator", "DispositivoVoto"
    };
    
    EXPECT_EQ(columns.size(), expected_columns.size());
    
    for (const auto& expected_col : expected_columns) {
        bool found = std::find(columns.begin(), columns.end(), expected_col) != columns.end();
        EXPECT_TRUE(found) << "Coluna '" << expected_col << "' não encontrada";
    }
}
