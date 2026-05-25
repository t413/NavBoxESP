#pragma once
#include <GeoPoint.h>

class TinyGPSPlus;
class HardwareSerial;

class GpsManager {
public:
    GpsManager();
    ~GpsManager();

    bool      hasFix()    const { return _hasFix; }
    double    lat()       const { return _currentPoint.lat; }
    double    lon()       const { return _currentPoint.lon; }
    float     alt()       const { return _currentPoint.alt; }
    float     speedMs()   const { return _currentPoint.speed; }
    float     hdop()      const { return _currentPoint.hdop; }
    uint8_t   satellites() const { return _satellites; }
    uint32_t  epoch()     const { return _epoch; }

    bool begin(HardwareSerial&);
    bool iterate(uint32_t now);
    TrackPoint toPoint() const { return _currentPoint; }

private:
    HardwareSerial* _serial = nullptr;
    TinyGPSPlus*    _gps;
    bool            _hasFix = false;
    uint32_t        _epoch  = 0;
    uint8_t         _satellites = 0;
    TrackPoint      _currentPoint{};

    bool _probe(int rx, int tx, uint32_t baud, HardwareSerial& uart);
    uint32_t _calcEpoch();
};
