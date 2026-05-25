#pragma once
#include <stdint.h>
#include <limits.h>
#include <cmath>


constexpr double NO_LOCATION = (double)(INT_MIN);
constexpr const double DEG_2_RAD = (M_PI / 180.0);

struct GeoPoint {
    float lat = 0, lon = 0;
    GeoPoint(double l = NO_LOCATION, double n = NO_LOCATION);
    void distHeadingPoint(double dist, double headingDeg, GeoPoint& out) const;
    operator bool() const;
    double distTo(const GeoPoint&) const;
    double approxDistTo(const GeoPoint&) const;
};


struct TrackPoint : public GeoPoint {
    TrackPoint(double l=0, double n=0, double a=0) : GeoPoint(l,n), alt(a) { }
    double verticalDistTo(const TrackPoint& o) const;
    // float    speed, hdop;
    float alt;
    uint32_t epoch;
    TrackPoint fromDistHeading(double dist, double headingDeg) const;
};
