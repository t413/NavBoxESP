#include "TrackLog.h"
#include "MapRenderer.h"
#include "fileclass.h"
#include <log.h>
#include <lvgl.h>

void TrackLog::clear() {
    path_.clear();
    bufferPos_ = 0;
    recordedPoints_ = 0;
    stats_ = {0};
    currentPath_[0] = 0;
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
    clear();
    MAP_LOG("track: load %s", path);
    strncpy(currentPath_, path, sizeof(currentPath_)); //save opened path
    clear();
    char line[128];
    isLoading_ = true;
    uint16_t totallines = 0;
    while (f.available()) {
        int len = f.readBytesUntil('\n', line, sizeof(line) - 1);
        line[len] = '\0';
        char* p = strstr(line, "<trkpt");
        if (p) {
            char* latStr = strstr(p, "lat=\"");
            char* lonStr = strstr(p, "lon=\"");
            if (latStr && lonStr) {
                TrackPoint pt(atof(latStr + 5), atof(lonStr + 5)); //TODO also parse epoc/alt
                addPoint(pt);
                if (recordedPoints_ % 50 == 0) {
                    MAP_LOG("track: load at %d (heap %d)", (int) path_.size(), freeHeap());
                    lv_timer_handler(); //periodically run the UI
                }
                #ifdef ARDUINO
                if (freeHeap() < 3000) {
                    MAP_LOG("track: load OOM aborting");
                    break;
                }
                #endif
            }
            totallines++;
        }
    }
    f.close();
    isLoading_ = false;
    MAP_LOG("track: done loading %d points from %d lines", (int) path_.size(), totallines);
    return true;
}

GeoPoint TrackLog::calcCenter() const {
    if (path_.empty()) return {};
    float minLat = 90, maxLat = -90, minLon = 180, maxLon = -180;
    for (const auto& p : path_) {
        auto lat = p.lat(), lon = p.lon();
        if (lat < minLat) minLat = lat;
        if (lat > maxLat) maxLat = lat;
        if (lon < minLon) minLon = lon;
        if (lon > maxLon) maxLon = lon;
    }
    return {
        (minLat + maxLat) * 0.5f,
        (minLon + maxLon) * 0.5f
    };
}

bool TrackLog::newRecording(uint32_t epoch) {
    if (isRecording_) return false;
    clear(); //clear any existing recording file

    if (!pathbase_) {
        MAP_LOG("track: rec without file base");
        return false;
    }
    if (!SD.exists(pathbase_)) SD.mkdir(pathbase_);

    char isoTime[24];
    _epochToIso(epoch, isoTime, sizeof(isoTime));
    for (int i = 0; isoTime[i]; i++)
        if (isoTime[i] == ':') isoTime[i] = '-'; // Replace colons with dashes for filename compatibility
    snprintf(currentPath_, sizeof(currentPath_), "%s/%s.gpx", pathbase_, isoTime);

    fs::File f = SD.open(currentPath_, FILE_WRITE);
    _writeHeader(f);
    _writeFooter(f);
    MAP_LOG("track: rec new file %s size %d", currentPath_, (int) f.position());
    f.close();
    isRecording_ = true;
    lastFlushTime_ = millis();
    return true;
}

bool TrackLog::recordResume(uint32_t epoch) {
    if (isRecording_) return false;
    if (currentPath_[0] != 0 && SD.exists(currentPath_)) {
        MAP_LOG("track: resuming %s", currentPath_);
        isRecording_ = true;
        lastFlushTime_ = millis();
    } else {
        newRecording(epoch);
    }
    return true;
}

void TrackLog::stopRecording() {
    if (!isRecording_) return;
    flush();
    isRecording_ = false;
    MAP_LOG("track: end rec to %s, wrote %d points", currentPath_, recordedPoints_);
}

bool TrackLog::addPoint(const TrackPoint& p) {
    if (!isRecording_ && !isLoading_) return false;
    if (!isLoading_) {
        if (bufferPos_ < BUFF_SIZE) //limit max size
            buffer_[bufferPos_++] = p; //all points
        if (bufferPos_ >= maxRawPoints_ || ((millis() - lastFlushTime_) > maxRawUnFlush_)) {
            flush();
        }
    }
    if (path_.empty() || path_.back().approxDistTo(p) > minDist_) {
        path_.push_back(p);
    }
    recordedPoints_++;
    stats_.update(p, lastPoint_);
    lastPoint_ = p;
    if ((recordedPoints_ % 75) == 0) {
        simplify();
    }
    return true;
}

void TrackLog::flush() {
    if (!bufferPos_ || !strnlen(currentPath_, sizeof(currentPath_))) {
        MAP_LOG("track::flush miss-call %s", currentPath_);
        return;
    }
    fs::File f = SD.open(currentPath_, "r+");
    if (!f) {
        MAP_LOG("track::flush err opening file %s", currentPath_);
        return;
    }
    uint32_t offset = _findTailOffset(f);
    MAP_LOG("flush offset %d", offset);
    if (offset == 0) {
        MAP_LOG("track: invalid flush offset %d", offset);
        return;
    }
    f.seek(offset);
    for (uint8_t i = 0; i < bufferPos_; i++) {
        _writePoint(f, buffer_[i]);
        buffer_[i] = TrackPoint(); //reset
    }
    _writeFooter(f);
    f.close();
    MAP_LOG("track: flushed %d points to %s (%d points overall)", bufferPos_, currentPath_, recordedPoints_);
    bufferPos_ = 0;
    lastFlushTime_ = millis();
}

void TrackLog::_writeHeader(fs::File& f) {
    f.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?><gpx version=\"1.1\" creator=\"NavBox\"><trk><trkseg>");
}

void TrackLog::_writePoint(fs::File& f, const TrackPoint& p) {
    char isoTime[24];
    _epochToIso(p.epoch, isoTime, sizeof(isoTime));
    f.printf("  <trkpt lat=\"%.7f\" lon=\"%.7f\"><ele>%.1f</ele><time>%s</time></trkpt>\n", p.lat(), p.lon(), p.alt, isoTime);
}

void TrackLog::_writeFooter(fs::File& f) {
    f.println("</trkseg></trk></gpx>");
}

bool TrackLog::shouldKeepPoint(size_t idx, const std::vector<GeoPoint>& path, float thresholdMeters) {
    if (idx == 0 || idx == path.size() - 1) return true; // always keep endpoints
    const auto& prev = path[idx - 1];
    const auto& curr = path[idx];
    const auto& next = path[idx + 1];
    if (!prev || !curr || !next) return true; // Skip if neighbors are invalid
    double dist = curr.lineDist(prev, next);
    return dist > thresholdMeters;
}

uint16_t TrackLog::simplifyRadial(std::vector<GeoPoint>& path, float thresholdMeters) {
    if (path.empty()) return 0;
    auto startsz = (uint16_t)path.size();
    // cull points
    for (size_t i = 1; i < path.size() - 1; ++i) {
        if (!shouldKeepPoint(i, path, thresholdMeters)) {
            path[i] = GeoPoint(); // invalidate
        }
    }
    // Compact in-place: move valid points to front
    size_t writeIdx = 0;
    for (size_t readIdx = 0; readIdx < path.size(); ++readIdx) {
        if (path[readIdx]) { // operator bool() checks NO_LOCATION
            path[writeIdx++] = path[readIdx];
        }
    }
    path.resize(writeIdx);
    MAP_LOG("simplifyRadial: %d -> %d points (cap %d)", startsz, (uint16_t) writeIdx, (uint16_t) path.capacity());
    return (uint16_t) writeIdx;
}

uint16_t TrackLog::simplify(float thresholdMeters) {
    return simplifyRadial(path_, thresholdMeters);
}

uint32_t TrackLog::_findTailOffset(fs::File& f) {
    constexpr uint8_t GOBACK = 45;
    uint32_t sz = f.size();
    if (sz < GOBACK) return sz;
    f.seek(sz - GOBACK);
    char buf[GOBACK] = {};
    f.readBytes(buf, GOBACK-1);
    char* p = strstr(buf, "</trkseg>");
    return p ? (sz - GOBACK + (p - buf)) : sz;
}


void TrackStats::update(const TrackPoint& point, const TrackPoint& prev) {
    if (!point) {
        return;
    }
    if (point.alt > maxAltitude) maxAltitude = point.alt;
    if (point.alt < minAltitude) minAltitude = point.alt;
    if (prev) {
        totalDist += prev.approxDistTo(point);
        float delta = point.alt - prev.alt;
        if (delta > 0) totalElevGain += delta;
        else totalElevLoss -= delta;
    }
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

    // First pass: count how many points project onto (or near) the screen
    // so we can compute a stride for uniform downsampling if needed.
    int visibleCount = 0;
    int firstVisible = -1, lastVisible = -1;
    for (int i = 0; i < (int)points.size(); ++i) {
        lv_coord_t px, py;
        renderer->project(points[i].lat(), points[i].lon(), px, py);
        if (px > -50 && px < renderer->width_ + 50 &&
            py > -50 && py < renderer->height_ + 50) {
            if (firstVisible == -1) firstVisible = i;
            lastVisible = i;
            visibleCount++;
        }
    }

    if (visibleCount == 0) {
        lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    // Stride: if more visible points than our budget, skip evenly.
    // stride=1 means full resolution (every point), which happens when
    // zoomed in and only a handful of points are on screen.
    constexpr int MAX_PTS = 25;
    int stride = (visibleCount + MAX_PTS - 1) / MAX_PTS; // ceiling div
    if (stride < 1) stride = 1;

    pointCount = 0;
    int skipped = 0;
    for (int i = firstVisible; i <= lastVisible && pointCount < MAX_PTS; ++i) {
        lv_coord_t px, py;
        renderer->project(points[i].lat(), points[i].lon(), px, py);
        bool onScreen = (px > -50 && px < renderer->width_ + 50 &&
                         py > -50 && py < renderer->height_ + 50);
        if (!onScreen) continue;
        if (skipped < stride - 1) { ++skipped; continue; }
        skipped = 0;
        lvPoints[pointCount].x = px;
        lvPoints[pointCount].y = py;
        pointCount++;
    }

    // Always include the last visible point so the line reaches the edge
    if (pointCount > 0 && lastVisible != -1) {
        lv_coord_t px, py;
        renderer->project(points[lastVisible].lat(), points[lastVisible].lon(), px, py);
        if (lvPoints[pointCount - 1].x != px || lvPoints[pointCount - 1].y != py) {
            if (pointCount < MAX_PTS) {
                lvPoints[pointCount].x = px;
                lvPoints[pointCount].y = py;
                pointCount++;
            } else {
                // Overwrite last slot with true endpoint
                lvPoints[pointCount - 1].x = px;
                lvPoints[pointCount - 1].y = py;
            }
        }
    }

    lv_line_set_points(line, lvPoints, pointCount);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_HIDDEN);
}
