#include "GeoPoint.h"

GeoPoint::GeoPoint(double l, double n, double a) : lat(l), lon(n), alt(a) { }
GeoPoint::operator bool() const { return lat != NO_LOCATION && lon != NO_LOCATION; }

constexpr double EARTH_RADIUS = 6371000; // in meters

double GeoPoint::distTo(const GeoPoint& o) const {
    //haversine formula
    double dLat = (o.lat - lat) * DEG_2_RAD;
    double dLon = (o.lon - lon) * DEG_2_RAD;
    double a = sin(dLat/2) * sin(dLat/2) + cos(lat * DEG_2_RAD) * cos(o.lat * DEG_2_RAD) * sin(dLon/2) * sin(dLon/2);
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

void GeoPoint::distHeadingPoint(double dist, double headingDeg, GeoPoint& out) const {
    double ad = dist / EARTH_RADIUS;
    double headingRad = headingDeg * DEG_2_RAD;
    double latRad = lat * DEG_2_RAD;
    double lonRad = lon * DEG_2_RAD;
    double outLatRad = asin(sin(latRad) * cos(ad) + cos(latRad) * sin(ad) * cos(headingRad));
    double outLonRad = lonRad + atan2(sin(headingRad) * sin(ad) * cos(latRad), cos(ad) - sin(latRad) * sin(outLatRad));
    out.lat = outLatRad / DEG_2_RAD;
    out.lon = outLonRad / DEG_2_RAD;
    out.alt = alt;
}

TrackPoint TrackPoint::fromDistHeading(double dist, double headingDeg) const {
    TrackPoint ret = *this;
    ret.distHeadingPoint(dist, headingDeg, ret);
    return ret;
}
