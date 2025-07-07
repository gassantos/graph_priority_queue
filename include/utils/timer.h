#ifndef UTILS_TIMER_H
#define UTILS_TIMER_H

#include <chrono>
#include <string>

/**
 * @file timer.h
 * @brief Utilitário para medição de tempo de execução
 * 
 * Fornece uma classe simples para medir tempos de execução
 * de operações no pipeline.
 */

namespace legal_doc_pipeline {
namespace utils {

    /**
     * @brief Classe para medição de tempo de execução
     */
    class Timer {
    private:
        std::chrono::high_resolution_clock::time_point start_time; ///< Tempo de início
        std::chrono::high_resolution_clock::time_point end_time;   ///< Tempo de fim
        bool is_running;                                           ///< Flag indicando se o timer está rodando

    public:
        /**
         * @brief Construtor
         */
        Timer();

        /**
         * @brief Destrutor
         */
        ~Timer() = default;

        /**
         * @brief Inicia a medição de tempo
         */
        void start();

        /**
         * @brief Para a medição de tempo
         */
        void stop();

        /**
         * @brief Reinicia o timer
         */
        void reset();

        /**
         * @brief Obtém o tempo decorrido em segundos
         * @return Tempo decorrido em segundos
         */
        double getElapsedSeconds() const;

        /**
         * @brief Obtém o tempo decorrido em milissegundos
         * @return Tempo decorrido em milissegundos
         */
        double getElapsedMilliseconds() const;

        /**
         * @brief Obtém uma representação em string do tempo decorrido
         * @return String formatada com o tempo decorrido
         */
        std::string getElapsedString() const;

        /**
         * @brief Verifica se o timer está rodando
         * @return true se o timer está rodando
         */
        bool isRunning() const;
    };

    /**
     * @brief Classe RAII para medição automática de tempo
     * 
     * Útil para medir o tempo de execução de um bloco de código
     * automaticamente usando RAII.
     */
    class ScopedTimer {
    private:
        Timer timer;           ///< Timer interno
        std::string name;      ///< Nome da operação sendo medida
        bool print_on_destroy; ///< Se deve imprimir o tempo no destrutor

    public:
        /**
         * @brief Construtor
         * @param operation_name Nome da operação sendo medida
         * @param print_result Se deve imprimir automaticamente o resultado
         */
        explicit ScopedTimer(const std::string& operation_name, bool print_result = true);

        /**
         * @brief Destrutor - imprime o tempo se configurado para tal
         */
        ~ScopedTimer();

        /**
         * @brief Obtém o timer interno
         * @return Referência para o timer
         */
        const Timer& getTimer() const;

        // Desabilita cópia e atribuição
        ScopedTimer(const ScopedTimer&) = delete;
        ScopedTimer& operator=(const ScopedTimer&) = delete;
    };

} // namespace utils
} // namespace legal_doc_pipeline

#endif // UTILS_TIMER_H
