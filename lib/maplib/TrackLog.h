#pragma once
#include <vector>
#include <string>
#include "GeoPoint.h"

namespace fs {
    class File;
}

struct TrackStats {
    float totalDist;
    float totalElevGain, totalElevLoss;
    float maxAltitude, minAltitude;
};

class TrackLog {
public:
    TrackLog(const char* pathbase=nullptr) : pathbase_(pathbase) { }
    void clear();
    size_t size() { return path_.size(); }
    bool load(const char* path);
    bool beginRecording(uint32_t epoch);
    void stopRecording();
    bool addPoint(const TrackPoint& p);
    void flush();

    const std::vector<GeoPoint>& points() const { return path_; }
    bool isRecording() const { return isRecording_; }
    const char* getRecPath() const { return currentPath_; }
    GeoPoint calcCenter() const;
    const TrackStats& getStats() const { return stats_; }

private:
    void _updateStats(const TrackPoint& point, const TrackPoint& prev);
    void _writeHeader(fs::File& f);
    void _writePoint(fs::File& f, const TrackPoint& p);
    void _writeFooter(fs::File& f);
    uint32_t _findTailOffset(fs::File& f);
    static void _epochToIso(uint32_t epoch, char* out, uint16_t outsize);
    bool _openFile(const char* path, const char* mode, fs::File& f);

    std::vector<GeoPoint> path_; //whole sparse track
    std::vector<TrackPoint> buffer_; //all raw points, written to GPX file
    TrackPoint lastPoint_;
    char currentPath_[64] = "";
    TrackStats stats_ = {0};
    bool isRecording_ = false;
    uint32_t lastFlushTime_ = 0;
    const char* pathbase_ = nullptr;
public:
    uint32_t recordedPoints_ = 0;
    double minDist_ = 10.0; //m
    uint16_t maxRawPoints_ = 10;
    uint32_t maxRawUnFlush_ = 15000; //ms
};

class MapRenderer;
struct _lv_obj_t;
#include <misc/lv_area.h>

struct TrackLogViewer {
    _lv_obj_t* line = nullptr;
    lv_point_t lvPoints[25];
    uint16_t pointCount = 0;

    TrackLogViewer(_lv_obj_t* parent, uint32_t linecolor);
    void update(MapRenderer*, TrackLog*);
};
