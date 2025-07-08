#ifndef UTILS_CSV_READER_H
#define UTILS_CSV_READER_H

#include <string>
#include <vector>
#include <map>

/**
 * @file csv_reader.h
 * @brief Utilitários para leitura de arquivos CSV
 * 
 * Este módulo fornece funcionalidades para ler e processar arquivos CSV,
 * específicamente otimizado para o dataset de documentos jurídicos.
 */

namespace legal_doc_pipeline {
namespace utils {

    /**
     * @brief Classe para leitura de arquivos CSV
     */
    class CsvReader {
    public:
        /**
         * @brief Construtor padrão
         */
        CsvReader() = default;

        /**
         * @brief Destrutor
         */
        ~CsvReader() = default;

        /**
         * @brief Lê uma coluna específica de um arquivo CSV
         * @param filename Caminho para o arquivo CSV
         * @param column_name Nome da coluna a ser lida
         * @param delimiter Delimitador usado no CSV (padrão: ';')
         * @return Vetor com os dados da coluna
         */
        std::vector<std::string> readColumn(
            const std::string& filename, 
            const std::string& column_name,
            char delimiter = ';'
        );

        /**
         * @brief Lê todas as colunas de um arquivo CSV
         * @param filename Caminho para o arquivo CSV
         * @param delimiter Delimitador usado no CSV (padrão: ';')
         * @return Mapa com nome da coluna e seus dados
         */
        std::map<std::string, std::vector<std::string>> readAllColumns(
            const std::string& filename,
            char delimiter = ';'
        );

        /**
         * @brief Valida se um arquivo CSV existe e é legível
         * @param filename Caminho para o arquivo CSV
         * @return true se o arquivo é válido
         */
        bool validateFile(const std::string& filename);

        /**
         * @brief Obtém os nomes das colunas de um arquivo CSV
         * @param filename Caminho para o arquivo CSV
         * @param delimiter Delimitador usado no CSV (padrão: ';')
         * @return Vetor com os nomes das colunas
         */
        std::vector<std::string> getColumnNames(
            const std::string& filename,
            char delimiter = ';'
        );

        /**
         * @brief Faz o parse de uma linha CSV (público para teste)
         * @param line Linha a ser processada
         * @param delimiter Delimitador usado
         * @return Vetor com as células da linha
         */
        std::vector<std::string> parseLine(const std::string& line, char delimiter);

    private:
        /**
         * @brief Remove aspas duplas de uma string, se presentes
         * @param str String a ser processada
         * @return String sem aspas duplas
         */
        std::string removeQuotes(const std::string& str);
    };

} // namespace utils
} // namespace legal_doc_pipeline

#endif // UTILS_CSV_READER_H
