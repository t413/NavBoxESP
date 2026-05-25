#include "TrackLog.h"
#include "MapRenderer.h"
#include "fileclass.h"
#include <log.h>
#include <lvgl.h>

void TrackLog::clear() {
    path_.clear();
    buffer_.clear();
    isRecording_ = false;
}

void TrackLog::_epochToIso(uint32_t epoch, char* out, uint16_t outsize) {
    if (epoch == 0) {
        strcpy(out, "2000-01-01T00:00:00Z");
        return;
    }
    time_t rawtime = (time_t)epoch;
    struct tm * ti;
#ifdef ARDUINO
    ti = gmtime(&rawtime);
#else
    ti = std::gmtime(&rawtime);
#endif
    snprintf(out, outsize, "%04d-%02d-%02dT%02d:%02d:%02dZ",
            ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
            ti->tm_hour, ti->tm_min, ti->tm_sec);
}

bool TrackLog::load(const char* path) {
    fs::File f = SD.open(path);
    if (!f) return false;
    MAP_LOG("TrackLog::load %s", path);
    clear();
    char line[128];
    while (f.available()) {
        int len = f.readBytesUntil('\n', line, sizeof(line) - 1);
        line[len] = '\0';
        char* p = strstr(line, "<trkpt");
        if (p) {
            TrackPoint pt{};
            char* latStr = strstr(p, "lat=\"");
            char* lonStr = strstr(p, "lon=\"");
            if (latStr && lonStr) {
                pt.lat = atof(latStr + 5);
                pt.lon = atof(lonStr + 5);
                if (path_.empty() || path_.back().approxDistTo(pt) > minDist_ * 2.0) {
                    path_.push_back(pt);
                    if (path_.size() % 10 == 0)
                        MAP_LOG("track load at %d (heap %d)", path_.size(), freeHeap());
                    #ifdef ARDUINO
                    if (freeHeap() < 3000) {
                        MAP_LOG("track load OOM aborting");
                        break;
                    }
                    #endif
                }
            }
        }
    }
    f.close();
    return true;
}

bool TrackLog::beginRecording(uint32_t epoch) {
    if (isRecording_) return false;
    if (pathbase_) {
        if (!SD.exists(pathbase_)) SD.mkdir(pathbase_);

        char isoTime[24];
        _epochToIso(epoch, isoTime, sizeof(isoTime));
        for (int i = 0; isoTime[i]; i++)
            if (isoTime[i] == ':') isoTime[i] = '-'; // Replace colons with dashes for filename compatibility
        snprintf(currentPath_, sizeof(currentPath_), "%s/%s.gpx", pathbase_, isoTime);

        fs::File f;
        if (SD.exists(currentPath_)) {
            f = SD.open(currentPath_, FILE_WRITE);
            f.seek(_findTailOffset(f));
        } else {
            f = SD.open(currentPath_, FILE_WRITE);
            _writeHeader(f);
        }
        _writeFooter(f);
        f.close();
        MAP_LOG("recording to %s", currentPath_);
    } else {
        MAP_LOG("recording without file");
    }
    isRecording_ = true;
    lastFlushTime_ = millis();
    recordedPoints_ = 0;
    return true;
}

void TrackLog::stopRecording() {
    if (!isRecording_) return;
    flush();
    isRecording_ = false;
    MAP_LOG("end recording to %s, wrote %d points", currentPath_, recordedPoints_);
}

bool TrackLog::addPoint(const TrackPoint& p) {
    if (!isRecording_) return false;
    buffer_.push_back(p); //all points
    recordedPoints_++;
    if (buffer_.size() >= maxRawPoints_ || ((millis() - lastFlushTime_) > maxRawUnFlush_)) {
        flush();
    }
    if (path_.empty() || path_.back().approxDistTo(p) > minDist_) {
        path_.push_back(p);
    }
    return true;
}

void TrackLog::flush() {
    if (buffer_.empty() || !strnlen(currentPath_, sizeof(currentPath_))) {
        MAP_LOG("tracklog::flush miss-call %s", currentPath_);
        return;
    }
    fs::File f = SD.open(currentPath_, FILE_WRITE);
    if (!f) {
        MAP_LOG("tracklog::flush err opening file %s", currentPath_);
        return;
    }
    uint32_t offset = _findTailOffset(f);
    f.seek(offset);
    for (const auto& p : buffer_) {
        _writePoint(f, p);
    }
    _writeFooter(f);
    f.close();
    MAP_LOG("flushed %d points to %s (%d points overall)", buffer_.size(), currentPath_, recordedPoints_);
    buffer_.clear();
    lastFlushTime_ = millis();
}

void TrackLog::_writeHeader(fs::File& f) {
    f.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?><gpx version=\"1.1\" creator=\"TrailPuter\"><trk><trkseg>");
}

void TrackLog::_writePoint(fs::File& f, const TrackPoint& p) {
    char isoTime[24];
    _epochToIso(p.epoch, isoTime, sizeof(isoTime));
    f.printf("  <trkpt lat=\"%.7f\" lon=\"%.7f\"><ele>%.1f</ele><time>%s</time></trkpt>\n", p.lat, p.lon, p.alt, isoTime);
}

void TrackLog::_writeFooter(fs::File& f) {
    f.println("</trkseg></trk></gpx>");
}

uint32_t TrackLog::_findTailOffset(fs::File& f) {
    constexpr uint8_t GOBACK = 45;
    uint32_t sz = f.size();
    if (sz < GOBACK) return sz;
    f.seek(sz - GOBACK);
    char buf[31] = {};
    f.readBytes(buf, GOBACK);
    char* p = strstr(buf, "</trkseg>");
    return p ? (sz - GOBACK + (p - buf)) : sz;
}


TrackLogViewer::TrackLogViewer(lv_obj_t* parent, uint32_t linecolor) {
    line = lv_line_create(parent);
    lv_obj_set_style_line_width(line, 3, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(linecolor), 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
}

void TrackLogViewer::update(MapRenderer* renderer, TrackLog* track) {
    if (!track || !line) return;
    const auto& points = track->points();
    if (points.empty()) {
        lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    // Find range of indices that are inside or near the viewport
    int first = -1, last = -1;
    for (int i = 0; i < (int)points.size(); ++i) {
        lv_coord_t px, py;
        renderer->project(points[i].lat, points[i].lon, px, py);
        // Use a bit of padding to include segments that cross the screen
        if (px > -50 && px < renderer->width_ + 50 && py > -50 && py < renderer->height_ + 50) {
            if (first == -1) first = i;
            last = i;
        }
    }

    if (first == -1) {
        lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    int rangeCount = last - first + 1;
    pointCount = 0;
    for (int i = 0; i < 25; i++) {
        if (i >= rangeCount) break;
        int idx = first + (i * (rangeCount - 1) / 24);
        lv_coord_t px, py;
        renderer->project(points[idx].lat, points[idx].lon, px, py);
        lvPoints[i].x = px;
        lvPoints[i].y = py;
        pointCount++;
    }

    lv_line_set_points(line, lvPoints, pointCount);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
}
