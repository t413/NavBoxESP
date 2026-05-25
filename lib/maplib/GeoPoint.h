#pragma once
#include <stdint.h>
#include <limits.h>

constexpr double NO_LOCATION = (double)(INT_MIN);

struct GeoPoint {
    double lat = 0, lon = 0, alt = 0;
    GeoPoint(double l = NO_LOCATION, double n = NO_LOCATION, double a = 0);
    operator bool() const;
    double distTo(const GeoPoint&) const;
    double approxDistTo(const GeoPoint&) const;
    double verticalDistTo(const GeoPoint& o) const;
};


struct TrackPoint : public GeoPoint {
    float    speed;
    float    hdop;
    uint32_t epoch;
};
