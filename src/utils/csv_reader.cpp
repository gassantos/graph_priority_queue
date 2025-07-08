#include "../../include/utils/csv_reader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

namespace legal_doc_pipeline {
namespace utils {

    std::vector<std::string> CsvReader::readColumn(
        const std::string& filename, 
        const std::string& column_name,
        char delimiter) {
        
        std::vector<std::string> column_data;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Erro ao abrir o arquivo CSV: " << filename << std::endl;
            return column_data;
        }

        std::string line;
        if (!std::getline(file, line)) {
            std::cerr << "Erro ao ler o cabeçalho do arquivo CSV" << std::endl;
            return column_data;
        }

        // Parse do cabeçalho
        line = removeBOM(line); // Remove BOM UTF-8 se presente
        std::vector<std::string> headers = parseLine(line, delimiter);
        int column_index = -1;
        
        for (size_t i = 0; i < headers.size(); ++i) {
            if (removeQuotes(headers[i]) == column_name) {
                column_index = static_cast<int>(i);
                break;
            }
        }

        if (column_index == -1) {
            std::cerr << "Coluna '" << column_name << "' não encontrada no CSV." << std::endl;
            return column_data;
        }

        // Lê os dados da coluna
        while (std::getline(file, line)) {
            std::vector<std::string> cells = parseLine(line, delimiter);
            if (column_index < static_cast<int>(cells.size())) {
                column_data.push_back(removeQuotes(cells[column_index]));
            } else {
                column_data.push_back(""); // Célula vazia se a linha não tem colunas suficientes
            }
        }
        
        file.close();
        return column_data;
    }

    std::map<std::string, std::vector<std::string>> CsvReader::readAllColumns(
        const std::string& filename,
        char delimiter) {
        
        std::map<std::string, std::vector<std::string>> data;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Erro ao abrir o arquivo CSV: " << filename << std::endl;
            return data;
        }

        std::string line;
        if (!std::getline(file, line)) {
            std::cerr << "Erro ao ler o cabeçalho do arquivo CSV" << std::endl;
            return data;
        }

        // Parse do cabeçalho
        line = removeBOM(line); // Remove BOM UTF-8 se presente
        std::vector<std::string> headers = parseLine(line, delimiter);
        
        // Inicializa os vetores para cada coluna
        for (const auto& header : headers) {
            data[removeQuotes(header)] = std::vector<std::string>();
        }

        // Lê os dados
        while (std::getline(file, line)) {
            std::vector<std::string> cells = parseLine(line, delimiter);
            
            for (size_t i = 0; i < headers.size(); ++i) {
                std::string header_name = removeQuotes(headers[i]);
                std::string cell_value = (i < cells.size()) ? removeQuotes(cells[i]) : "";
                data[header_name].push_back(cell_value);
            }
        }
        
        file.close();
        return data;
    }

    bool CsvReader::validateFile(const std::string& filename) {
        std::ifstream file(filename);
        return file.is_open() && file.good();
    }

    std::vector<std::string> CsvReader::getColumnNames(
        const std::string& filename,
        char delimiter) {
        
        std::vector<std::string> column_names;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Erro ao abrir o arquivo CSV: " << filename << std::endl;
            return column_names;
        }

        std::string line;
        if (std::getline(file, line)) {
            line = removeBOM(line); // Remove BOM UTF-8 se presente
            std::vector<std::string> headers = parseLine(line, delimiter);
            for (const auto& header : headers) {
                column_names.push_back(removeQuotes(header));
            }
        }
        
        file.close();
        return column_names;
    }

    std::string CsvReader::removeQuotes(const std::string& str) {
        // Como o parser já remove as aspas, só precisamos remover espaços e caracteres de controle
        std::string result = str;
        // Remove espaços e caracteres de controle no início e no final (incluindo \r, \n)
        size_t start = result.find_first_not_of(" \t\r\n");
        if (start != std::string::npos) {
            result = result.substr(start);
        }
        size_t end = result.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            result = result.substr(0, end + 1);
        }
        return result;
    }

    std::vector<std::string> CsvReader::parseLine(const std::string& line, char delimiter) {
        std::vector<std::string> cells;
        std::string current_cell;
        bool in_quotes = false;
        
        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];
            
            if (c == '"') {
                in_quotes = !in_quotes;
                // Não adiciona as aspas ao conteúdo da célula
            } else if (c == delimiter && !in_quotes) {
                cells.push_back(current_cell);
                current_cell.clear();
            } else {
                current_cell += c;
            }
        }
        
        // Adiciona a última célula
        cells.push_back(current_cell);
        
        return cells;
    }

    std::string CsvReader::removeBOM(const std::string& str) {
        // BOM UTF-8 são os bytes: 0xEF 0xBB 0xBF
        if (str.size() >= 3 && 
            static_cast<unsigned char>(str[0]) == 0xEF &&
            static_cast<unsigned char>(str[1]) == 0xBB &&
            static_cast<unsigned char>(str[2]) == 0xBF) {
            return str.substr(3);
        }
        return str;
    }

} // namespace utils
} // namespace legal_doc_pipeline
