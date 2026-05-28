#include "MapLayer.h"
#include "MapRenderer.h"
#include "log.h"
#include "TrackLog.h"
#include "fileclass.h"
#include <lvgl.h>


MarkerLayer::MarkerLayer(MapRenderer* map) : map_(map) {}

MarkerLayer::~MarkerLayer() {
    for (auto& m : markers_) {
        if (m.obj_) lv_obj_del(m.obj_);
    }
}

uint16_t MarkerLayer::add(const Marker& marker) {
    MarkObj m = {marker, count_++, lv_obj_create(map_->getLvglBase())};
    lv_obj_set_style_radius(m.obj_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(m.obj_, lv_color_white(), 0);
    lv_obj_set_style_border_width(m.obj_, 2, 0);
    if (marker.label != 0) {
        char buf[2] = {marker.label, 0};
        auto lbl = lv_label_create(m.obj_);
        lv_label_set_text(lbl, buf);
        lv_obj_center(lbl);
    }
    markers_.push_back(m);
    updateOne(m);
    return m.id;
}

bool MarkerLayer::updatePoint(uint16_t id, const GeoPoint &gp) {
    for (auto& m : markers_) {
        if (m.id == id) {
            m.m_.pos = gp;
            return true;
        }
    }
    return false;
}

const MarkerLayer::MarkObj MarkerLayer::empty = MarkObj();

const Marker& MarkerLayer::get(uint16_t id) const {
    for (const auto& m : markers_)
        if (m.id == id)
            return m.m_;
    return MarkerLayer::empty.m_;
}

void MarkerLayer::remove(uint16_t id) {
    auto it = std::find_if(markers_.begin(), markers_.end(),
        [id](const MarkObj& m) { return m.id == id; });
    if (it != markers_.end()) {
        if (it->obj_) lv_obj_del(it->obj_);
        markers_.erase(it);
    }
}

void MarkerLayer::updateOne(MarkObj& m) {
    if (!m.obj_) return;
    lv_coord_t px, py;
    if (!m.m_.pos || !map_->project(m.m_.pos.lat(), m.m_.pos.lon(), px, py)) {
        lv_obj_add_flag(m.obj_, LV_OBJ_FLAG_HIDDEN);
    } else {
        uint16_t sz = m.m_.size;
        lv_obj_set_size(m.obj_, sz, sz);
        lv_obj_set_pos(m.obj_, px - sz/2, py - sz/2);
        lv_obj_set_style_bg_color(m.obj_, lv_color_hex(m.m_.color), 0);
        lv_obj_clear_flag(m.obj_, LV_OBJ_FLAG_HIDDEN);
    }
}

void MarkerLayer::update() {
    for (auto& m : markers_) {
        updateOne(m);
    }
}

TrackLayer::TrackLayer(MapRenderer* map, uint32_t color, TrackLog* track) {
    map_ = map;
    track_ = track;
    line = lv_line_create(map->getLvglBase());
    lv_obj_set_style_line_width(line, 3, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(color), 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
}

TrackLayer::~TrackLayer() {
    if (line) lv_obj_del(line);
}

void TrackLayer::update() {
    if (!track_ || !line) return;
    const auto& points = track_->points();
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
        map_->project(points[i].lat(), points[i].lon(), px, py);
        if (px > -50 && px < map_->getWidth() + 50 &&
            py > -50 && py < map_->getHeight() + 50) {
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
        map_->project(points[i].lat(), points[i].lon(), px, py);
        bool onScreen = (px > -50 && px < map_->getWidth() + 50 &&
                         py > -50 && py < map_->getHeight() + 50);
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
        map_->project(points[lastVisible].lat(), points[lastVisible].lon(), px, py);
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
