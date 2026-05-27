#include "GpsManager.h"
#include <log.h>
#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <M5Unified.h>

GpsManager::GpsManager() {
    _gps = new TinyGPSPlus();
}

GpsManager::~GpsManager() {
    delete _gps;
}

bool GpsManager::begin(HardwareSerial& port) {
    std::vector<std::pair<int, int>> probes;
    auto brd = M5.getBoard();

    if (brd == m5gfx::board_t::board_M5Cardputer || brd == m5gfx::board_t::board_M5CardputerADV) {
        probes = {{1, 2}, {13, 15}}; //grove, then lora cap
    } else {
        probes = {{1, 2}}; // Default Grove
    }

    MAP_LOG("gps probing %d pairs", probes.size());
    for (auto& p : probes) {
        if (_probe(p.first, p.second, 115200, port)) {
            MAP_LOG("gps found GPS on [%d,%d]", p.first, p.second);
            _serial = &port;
            return true;
        }
    }
    return false;
}

bool GpsManager::iterate(uint32_t now) {
    if (!_serial) return false;
    bool newFix = false;
    while (_serial->available()) {
        if (_gps->encode(_serial->read())) {
            if (_gps->location.isValid() && _gps->location.isUpdated()) {
                _hasFix = true;
                _epoch = _calcEpoch();
                _satellites = (uint8_t)_gps->satellites.value();

                _currentPoint = {_gps->location.lat(), _gps->location.lng()};
                _currentPoint.alt = _gps->altitude.meters();
                speed_ = _gps->speed.mps();
                hdoop_ = (_gps->hdop.isValid() ? _gps->hdop.hdop() : 99.9f);
                _currentPoint.epoch = _epoch;

                newFix = true;
            }
        }
     }
    return newFix;
}

bool GpsManager::_probe(int rx, int tx, uint32_t baud, HardwareSerial& uart) {
    uart.begin(baud, SERIAL_8N1, rx, tx);
    uint32_t t = millis();
    while (millis() - t < 1200) {
        while (uart.available()) {
            if (uart.read() == '$') return true;
        }
        delay(10);
    }
    uart.end();
    return false;
}

uint32_t GpsManager::_calcEpoch() {
    if (!_gps->date.isValid() || !_gps->time.isValid()) return millis() / 1000;
    uint16_t y = _gps->date.year();
    uint8_t  m = _gps->date.month();
    uint8_t  d = _gps->date.day();
    static const uint16_t daysPerMonth[] = {0,31,59,90,120,151,181,212,243,273,304,334};
    uint32_t days = (y - 1970) * 365UL + (y - 1969) / 4 - (y - 1901) / 100 + (y - 1601) / 400 + daysPerMonth[m-1] + d - 1;
    if (m > 2 && ((y%4==0 && y%100!=0) || y%400==0)) days++;
    return days * 86400UL + _gps->time.hour() * 3600UL + _gps->time.minute() * 60UL + _gps->time.second();
}
