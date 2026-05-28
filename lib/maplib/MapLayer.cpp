#include "MapLayer.h"
#include "MapRenderer.h"
#include "log.h"
#include "TrackLog.h"
#include "fileclass.h"
#include <lvgl.h>


MarkerLayer::MarkerLayer(lv_obj_t* parent) : parent_(parent) {}

MarkerLayer::~MarkerLayer() {
    MAP_LOG("~MarkerLayer");
    for (auto& obj : objs_) {
        if (obj) lv_obj_del(obj);
    }
}

Marker* MarkerLayer::addMarker(const Marker& m) {
    markers_.push_back(m);
    return &(markers_.back());
}
void MarkerLayer::removeMarker(size_t idx) {
    if (idx < markers_.size()) {
        if (idx < objs_.size() && objs_[idx]) {
            lv_obj_del(objs_[idx]);
            objs_[idx] = nullptr;
        }
        markers_.erase(markers_.begin() + idx);
    }
}

void MarkerLayer::update(MapRenderer* renderer) {
    objs_.resize(markers_.size(), nullptr);
    for (size_t i = 0; i < markers_.size(); i++) {
        lv_coord_t px, py;
        if (!renderer->project(markers_[i].pos.lat(), markers_[i].pos.lon(), px, py)) {
            if (objs_[i]) lv_obj_add_flag(objs_[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        if (!objs_[i]) {
            auto mobj = lv_obj_create(parent_);
            lv_obj_set_style_radius(mobj, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_border_color(mobj, lv_color_white(), 0);
            lv_obj_set_style_border_width(mobj, 2, 0);
            if (markers_[i].label != 0) {
                char buf[2] = {markers_[i].label};
                auto lbl = lv_label_create(mobj);
                lv_label_set_text(lbl, buf);
                lv_obj_center(lbl);
            }
            objs_[i] = mobj;
        }

        uint16_t sz = markers_[i].size;
        lv_obj_set_size(objs_[i], sz, sz);
        lv_obj_set_pos(objs_[i], px - sz/2, py - sz/2);
        lv_obj_set_style_bg_color(objs_[i], lv_color_hex(markers_[i].color), 0);
        lv_obj_clear_flag(objs_[i], LV_OBJ_FLAG_HIDDEN);
    }
}

TrackLayer::TrackLayer(lv_obj_t* parent, uint32_t color, TrackLog* track) {
    line = lv_line_create(parent);
    lv_obj_set_style_line_width(line, 3, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(color), 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_add_flag(line, LV_OBJ_FLAG_HIDDEN);
}

TrackLayer::~TrackLayer() {
    MAP_LOG("~TrackLayer");
    if (line) lv_obj_del(line);
}

void TrackLayer::update(MapRenderer* renderer) {
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
        renderer->project(points[i].lat(), points[i].lon(), px, py);
        if (px > -50 && px < renderer->getWidth() + 50 &&
            py > -50 && py < renderer->getHeight() + 50) {
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
        bool onScreen = (px > -50 && px < renderer->getWidth() + 50 &&
                         py > -50 && py < renderer->getHeight() + 50);
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
