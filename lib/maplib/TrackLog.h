#pragma once
#include <vector>
#include <string>
#include "GeoPoint.h"
#include <Arduino.h>
#include <SD.h>

class TrackLog {
public:
    void clear();
    bool load(const char* path);
    bool beginRecording(const char* path);
    void stopRecording();
    bool addPoint(const TrackPoint& p);
    void flush();

    const std::vector<TrackPoint>& points() const { return path_; }
    bool isRecording() const { return isRecording_; }

private:
    void _writeHeader(File& f);
    void _appendPoint(File& f, const TrackPoint& p);
    void _writeFooter(File& f);
    uint32_t _findTailOffset(File& f);
    void _epochToIso(uint32_t epoch, char* out);

    std::vector<TrackPoint> path_;
    std::vector<TrackPoint> buffer_;
    std::string currentPath_;
    bool isRecording_ = false;
    uint32_t lastFlushTime_ = 0;
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

