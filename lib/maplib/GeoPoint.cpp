#include "GeoPoint.h"
#include <algorithm>

GeoPoint::GeoPoint(double l, double n) : lat_e7((int32_t)(l * GEO_E7)), lon_e7((int32_t)(n * GEO_E7)) { }
GeoPoint::operator bool() const { return lat_e7 != NO_LOCATION && lon_e7 != NO_LOCATION; }

constexpr double EARTH_RADIUS = 6371000; // in meters

double GeoPoint::lat() const { return (double)lat_e7 / GEO_E7; }
double GeoPoint::lon() const { return (double)lon_e7 / GEO_E7; }

double GeoPoint::distTo(const GeoPoint& o) const {
    //haversine formula
    double dLat = (o.lat() - lat()) * DEG_2_RAD;
    double dLon = (o.lon() - lon()) * DEG_2_RAD;
    double a = sin(dLat/2) * sin(dLat/2) + cos(lat() * DEG_2_RAD) * cos(o.lat() * DEG_2_RAD) * sin(dLon/2) * sin(dLon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return EARTH_RADIUS * c; // Earth radius in meters
}

double GeoPoint::approxDistTo(const GeoPoint& o) const {
    // equirectangular approximation, faster but less accurate
    double x = (o.lon() - lon()) * cos((lat() + o.lat()) * M_PI / 360.0);
    double y = (o.lat() - lat());
    return sqrt(x*x + y*y) * M_PI * EARTH_RADIUS / 180.0;
}

float GeoPoint::lineDist(const GeoPoint& a, const GeoPoint& b) const {
    // Vector from a to b
    float px = b.lon() - a.lon();
    float py = b.lat() - a.lat();
    float plen2 = px*px + py*py;
    if (plen2 < 1e-18) return distTo(a); // degenerate: a ≈ b
    // Vector from a to this point
    float ax = lon() - a.lon();
    float ay = lat() - a.lat();
    // Project onto line, clamp to [0, 1]
    float t = (ax*px + ay*py) / plen2;
    t = std::max(0.0f, std::min(1.0f, t));
    // Closest point on segment
    GeoPoint closest(a.lat() + t*py, a.lon() + t*px);
    return distTo(closest);
}

void GeoPoint::distHeadingPoint(double dist, double headingDeg, GeoPoint& out) const {
    double ad = dist / EARTH_RADIUS;
    double headingRad = headingDeg * DEG_2_RAD;
    double latRad = lat() * DEG_2_RAD;
    double lonRad = lon() * DEG_2_RAD;
    double outLatRad = asin(sin(latRad) * cos(ad) + cos(latRad) * sin(ad) * cos(headingRad));
    double outLonRad = lonRad + atan2(sin(headingRad) * sin(ad) * cos(latRad), cos(ad) - sin(latRad) * sin(outLatRad));
    out.lat_e7 = (int32_t)(outLatRad / DEG_2_RAD * GEO_E7);
    out.lon_e7 = (int32_t)(outLonRad / DEG_2_RAD * GEO_E7);
}

TrackPoint TrackPoint::fromDistHeading(double dist, double headingDeg) const {
    TrackPoint ret = *this;
    ret.distHeadingPoint(dist, headingDeg, ret);
    return ret;
}

double TrackPoint::verticalDistTo(const TrackPoint& o) const { return (o.alt - alt); }
