#pragma once
#include <stdint.h>
#include <limits.h>
#include <cmath>


constexpr int NO_LOCATION = (INT_MIN);
constexpr const double DEG_2_RAD = (M_PI / 180.0);
constexpr const double GEO_E7 = 1e7;

struct GeoPoint {
    int32_t lat_e7, lon_e7;  // latitude * 10^7 (sub-meter precision)
    GeoPoint(double l = NO_LOCATION, double n = NO_LOCATION);
    void distHeadingPoint(double dist, double headingDeg, GeoPoint& out) const;
    operator bool() const;
    double lat() const;
    double lon() const;
    double distTo(const GeoPoint&) const;
    double approxDistTo(const GeoPoint&) const;
    float lineDist(const GeoPoint& a, const GeoPoint& b) const;
};


struct TrackPoint : public GeoPoint {
    TrackPoint(double l = NO_LOCATION, double n = NO_LOCATION, double a=0) : GeoPoint(l,n), alt(a) { }
    double verticalDistTo(const TrackPoint& o) const;
    // float    speed, hdop;
    float alt = 0.0;
    uint32_t epoch = 0;
    TrackPoint fromDistHeading(double dist, double headingDeg) const;
};
