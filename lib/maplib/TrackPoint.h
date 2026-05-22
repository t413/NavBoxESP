#pragma once
#include <stdint.h>

struct TrackPoint {
    double   lat;
    double   lon;
    float    alt;
    float    speed;
    float    hdop;
    uint32_t epoch;
};
