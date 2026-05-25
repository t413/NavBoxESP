#include "TrackLog.h"
#include "MapRenderer.h"
#include <log.h>
#include <lvgl.h>

void TrackLog::clear() {
    path_.clear();
    buffer_.clear();
    isRecording_ = false;
}

bool TrackLog::load(const char* path) {
    File f = SD.open(path, FILE_READ);
    if (!f) return false;
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
                addPoint(pt);
            }
        }
    }
    f.close();
    return true;
}

bool TrackLog::beginRecording(const char* path) {
    if (isRecording_) return false;
    currentPath_ = path;
    if (!SD.exists("/tracks")) SD.mkdir("/tracks");

    File f;
    if (SD.exists(path)) {
        f = SD.open(path, FILE_WRITE);
        uint32_t offset = _findTailOffset(f);
        f.seek(offset);
    } else {
        f = SD.open(path, FILE_WRITE);
        _writeHeader(f);
    }
    _writeFooter(f);
    f.close();
    isRecording_ = true;
    lastFlushTime_ = millis();
    return true;
}

void TrackLog::stopRecording() {
    if (!isRecording_) return;
    flush();
    isRecording_ = false;
}

bool TrackLog::addPoint(const TrackPoint& p) {
    if (!path_.empty()) {
        if (path_.back().approxDistTo(p) < 10.0f) return false;
    }
    path_.push_back(p);
    if (isRecording_) {
        buffer_.push_back(p);
        if (buffer_.size() >= 10 || (millis() - lastFlushTime_ > 30000)) {
            flush();
        }
    }
    return true;
}

void TrackLog::flush() {
    if (buffer_.empty() || currentPath_.empty()) return;
    File f = SD.open(currentPath_.c_str(), FILE_WRITE);
    if (!f) return;
    uint32_t offset = _findTailOffset(f);
    f.seek(offset);
    for (const auto& p : buffer_) {
        _appendPoint(f, p);
    }
    _writeFooter(f);
    f.close();
    buffer_.clear();
    lastFlushTime_ = millis();
}

void TrackLog::_writeHeader(File& f) {
    f.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?><gpx version=\"1.1\" creator=\"TrailPuter\"><trk><trkseg>");
}

void TrackLog::_appendPoint(File& f, const TrackPoint& p) {
    f.printf("  <trkpt lat=\"%.7f\" lon=\"%.7f\"><ele>%.1f</ele></trkpt>\n", p.lat, p.lon, p.alt);
}

void TrackLog::_writeFooter(File& f) {
    f.println("</trkseg></trk></gpx>");
}

uint32_t TrackLog::_findTailOffset(File& f) {
    uint32_t sz = f.size();
    if (sz < 30) return sz;
    f.seek(sz - 30);
    char buf[31] = {};
    f.readBytes(buf, 30);
    char* p = strstr(buf, "</trkseg>");
    return p ? (sz - 30 + (p - buf)) : sz;
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
