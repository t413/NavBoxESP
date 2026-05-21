#pragma once

// ── Debug Logging ────────────────────────────────────────────────────────────
#ifdef ARDUINO
    #include <Arduino.h>
    #define MAP_LOG(fmt, ...) Serial.printf("[MAP] " fmt, ##__VA_ARGS__)
#else
    #include <cstdio>
    #define MAP_LOG(fmt, ...) printf("[MAP] " fmt, ##__VA_ARGS__)
#endif
