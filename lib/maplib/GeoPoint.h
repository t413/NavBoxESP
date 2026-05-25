#pragma once
#include <stdint.h>
#include <limits.h>
#include <cmath>


constexpr double NO_LOCATION = (double)(INT_MIN);
constexpr const double DEG_2_RAD = (M_PI / 180.0);

struct GeoPoint {
    double lat = 0, lon = 0, alt = 0;
    GeoPoint(double l = NO_LOCATION, double n = NO_LOCATION, double a = 0);
    void distHeadingPoint(double dist, double headingDeg, GeoPoint& out) const;
    operator bool() const;
    double distTo(const GeoPoint&) const;
    double approxDistTo(const GeoPoint&) const;
    double verticalDistTo(const GeoPoint& o) const;
};


struct TrackPoint : public GeoPoint {
    TrackPoint(double l=0, double n=0, double a=0) : GeoPoint(l,n,a) { }
    float    speed;
    float    hdop;
    uint32_t epoch;
    TrackPoint fromDistHeading(double dist, double headingDeg) const;
};
