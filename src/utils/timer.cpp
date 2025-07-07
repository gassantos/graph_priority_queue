#include "../../include/utils/timer.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace legal_doc_pipeline {
namespace utils {

    Timer::Timer() : is_running(false) {}

    void Timer::start() {
        start_time = std::chrono::high_resolution_clock::now();
        is_running = true;
    }

    void Timer::stop() {
        if (is_running) {
            end_time = std::chrono::high_resolution_clock::now();
            is_running = false;
        }
    }

    void Timer::reset() {
        is_running = false;
        start_time = std::chrono::high_resolution_clock::time_point{};
        end_time = std::chrono::high_resolution_clock::time_point{};
    }

    double Timer::getElapsedSeconds() const {
        auto end = is_running ? std::chrono::high_resolution_clock::now() : end_time;
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_time);
        return duration.count() / 1000000.0;
    }

    double Timer::getElapsedMilliseconds() const {
        auto end = is_running ? std::chrono::high_resolution_clock::now() : end_time;
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_time);
        return duration.count() / 1000.0;
    }

    std::string Timer::getElapsedString() const {
        double seconds = getElapsedSeconds();
        std::stringstream ss;
        
        if (seconds < 1.0) {
            ss << std::fixed << std::setprecision(2) << getElapsedMilliseconds() << " ms";
        } else if (seconds < 60.0) {
            ss << std::fixed << std::setprecision(3) << seconds << " s";
        } else {
            int minutes = static_cast<int>(seconds) / 60;
            double remaining_seconds = seconds - (minutes * 60);
            ss << minutes << "m " << std::fixed << std::setprecision(2) << remaining_seconds << "s";
        }
        
        return ss.str();
    }

    bool Timer::isRunning() const {
        return is_running;
    }

    // ScopedTimer implementation
    ScopedTimer::ScopedTimer(const std::string& operation_name, bool print_result)
        : name(operation_name), print_on_destroy(print_result) {
        timer.start();
    }

    ScopedTimer::~ScopedTimer() {
        timer.stop();
        if (print_on_destroy) {
            std::cout << "[Timer] " << name << " concluído em " << timer.getElapsedString() << std::endl;
        }
    }

    const Timer& ScopedTimer::getTimer() const {
        return timer;
    }

} // namespace utils
} // namespace legal_doc_pipeline
