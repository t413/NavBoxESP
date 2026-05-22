#pragma once

// ── Debug Logging ────────────────────────────────────────────────────────────
#ifdef ARDUINO
    #include <Arduino.h>
    #define MAP_LOG(fmt, ...) Serial.printf("[MAP] " fmt "\n", ##__VA_ARGS__)
#else
    #include <cstdio>
    #define MAP_LOG(fmt, ...) { printf("[MAP] " fmt "\n", ##__VA_ARGS__); fflush(stdout); }
#endif
