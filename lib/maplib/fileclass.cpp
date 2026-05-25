#include "fileclass.h"

#ifndef ARDUINO
#include <chrono>

SDClass SD;

uint32_t millis() {
    // Simple millis implementation using C++11 chrono
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return duration_cast<milliseconds>(steady_clock::now() - start).count();
}

#endif
