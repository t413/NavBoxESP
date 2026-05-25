#include "GeoPoint.h"
#include <cmath>

GeoPoint::GeoPoint(double l, double n, double a) : lat(l), lon(n), alt(a) { }
GeoPoint::operator bool() const { return lat != NO_LOCATION && lon != NO_LOCATION; }

constexpr double EARTH_RADIUS = 6371000; // in meters

double GeoPoint::distTo(const GeoPoint& o) const {
    //haversine formula
    double dLat = (o.lat - lat) * M_PI / 180.0;
    double dLon = (o.lon - lon) * M_PI / 180.0;
    double a = sin(dLat/2) * sin(dLat/2) + cos(lat * M_PI / 180.0) * cos(o.lat * M_PI / 180.0) * sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return EARTH_RADIUS * c; // Earth radius in meters
}

double GeoPoint::approxDistTo(const GeoPoint& o) const {
    // equirectangular approximation, faster but less accurate
    double x = (o.lon - lon) * cos((lat + o.lat) * M_PI / 360.0);
    double y = (o.lat - lat);
    return sqrt(x*x + y*y) * M_PI * EARTH_RADIUS / 180.0;
}

double GeoPoint::verticalDistTo(const GeoPoint& o) const { return (o.alt - alt); }
