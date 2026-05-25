#include "MapView.h"
#include <log.h>
#include "controller.h"
#include "GpsManager.h"

// Local constants to replace Include/Config.h
static constexpr int SIDEBAR_W = 60;
static constexpr int SCREEN_H  = 135;
static constexpr uint32_t COL_SIDEBAR_BG = 0x161B22; //161B22
static constexpr uint32_t COL_TEXT       = 0xE6EDF3; //E6EDF3
static constexpr uint32_t COL_TEXT_DIM   = 0x8B949E; //ccd4dc
static constexpr uint32_t COL_ACCENT     = 0x58A6FF; //58A6FF

void MapView::create(lv_obj_t* parent, Controller* ctrl) {
    root_ = lv_obj_create(parent);
    ctrl_ = ctrl;
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);

    // Initialize Map on the left side
    map_.begin(root_, 240 - SIDEBAR_W, 135, BASEDIR_TILES "/%d/%d/%d.png");
    map_.setXY(0, 0);
    map_.setZoom(14);

    _createSidebar(root_);
    map_.setTracks(&(ctrl->recordTrack_), &(ctrl->viewTrack_));
    _updateSidebar();
}

void MapView::show() { lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN); }
void MapView::hide() { lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN); }

void MapView::update(bool inview) {
    isActive_ = inview;
}

void MapView::onGPSUpdate(GpsManager* gps) {
    if (!gps) return;
    if (!gps_) gps_ = gps;
    auto point = gps->toPoint();
    _updateSidebar(&point);

    map_.setDot(point.lat, point.lon);

    if (followMode_ && gps->hasFix()) {
        bool needsCenter = true;
        if (map_.cropmode_) {
            lv_coord_t px, py;
            map_.project(point.lat, point.lon, px, py);
            if (map_.isVisible(px, py)) { //TODO recenter more often, but not too often
                needsCenter = false;
            }
        }

        if (needsCenter) {
            map_.setCenter(point.lat, point.lon);
        }
    }
}

void MapView::onKey(uint8_t key) {
    switch (key) {
        case ctrlbtns::KEY_ARROW_UP:
            map_.panPx(0, -20); followMode_ = false; break;
        case ctrlbtns::KEY_ARROW_DOWN:
            map_.panPx(0, 20);  followMode_ = false; break;
        case ctrlbtns::KEY_ARROW_LEFT:
            map_.panPx(-20, 0); followMode_ = false; break;
        case ctrlbtns::KEY_ARROW_RIGHT:
            map_.panPx(20, 0);  followMode_ = false; break;
        case '[': map_.setZoom(map_.zoom() - 1); break;
        case ']': map_.setZoom(map_.zoom() + 1); break;
        case 'f':
            followMode_ = true;
            if (gps_ && gps_->hasFix())
                map_.setCenter(gps_->lat(), gps_->lon());
            break;
        case 'h':
            if (gps_ && gps_->hasFix())
                map_.setHome(gps_->lat(), gps_->lon());
            break;
        case 'r':
            if (ctrl_) ctrl_->toggleRecording();
            break;
    }
}

bool MapView::handleBack() {
    if (!followMode_)
        return ((followMode_ = true));
    return false;
}

void MapView::_createSidebar(lv_obj_t* parent) {
    sidebar_ = lv_obj_create(parent);
    lv_obj_set_size(sidebar_, SIDEBAR_W, SCREEN_H);
    lv_obj_set_pos(sidebar_, 240 - SIDEBAR_W, 0);
    lv_obj_set_style_bg_color(sidebar_, lv_color_hex(COL_SIDEBAR_BG), 0);
    lv_obj_set_style_border_side(sidebar_, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(sidebar_, lv_color_hex(0x2a3040), 0);
    lv_obj_set_style_border_width(sidebar_, 1, 0);
    lv_obj_set_style_pad_all(sidebar_, 0, 0);

    gpsDot_ = _makeDot(sidebar_, 6, 6);
    satLabel_ = _makeLabel(sidebar_, 16, 2, &lv_font_montserrat_12);
    lv_obj_set_style_text_color(satLabel_, lv_color_hex(COL_TEXT), 0);

    battLabel_ = _makeLabel(sidebar_, 38, 2, &lv_font_montserrat_12);
    lv_obj_set_style_text_color(battLabel_, lv_color_hex(COL_TEXT), 0);

    speedLabel_ = _makeLabel(sidebar_, 2, 18, &lv_font_montserrat_14);
    lv_obj_set_width(speedLabel_, SIDEBAR_W - 4);
    lv_obj_set_style_text_color(speedLabel_, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_text_align(speedLabel_, LV_TEXT_ALIGN_CENTER, 0);

    _makeDivider(sidebar_, 46);
    altLabel_ = _makeLabel(sidebar_, 2, 60, &lv_font_montserrat_12);
    lv_obj_set_width(altLabel_, SIDEBAR_W - 4);
    lv_obj_set_style_text_align(altLabel_, LV_TEXT_ALIGN_CENTER, 0);

    _makeDivider(sidebar_, 76);
    distLabel_ = _makeLabel(sidebar_, 2, 90, &lv_font_montserrat_12);
    lv_obj_set_width(distLabel_, SIDEBAR_W - 4);
    lv_obj_set_style_text_align(distLabel_, LV_TEXT_ALIGN_CENTER, 0);

    _makeDivider(sidebar_, 106);
    recDot_ = _makeDot(sidebar_, 6, 112);
    recLabel_ = _makeLabel(sidebar_, 16, 110, &lv_font_montserrat_10);

    for (int i = 0; i < (int)ViewID::COUNT; i++) {
        viewDots_[i] = _makeDot(sidebar_, 4 + i * 10, SCREEN_H - 10);
        lv_obj_set_size(viewDots_[i], 6, 6);
    }
}

void MapView::_updateSidebar(const TrackPoint* point) {
    bool fixed = gps_ && gps_->hasFix();
    int hdop = gps_ && gps_->hdop();
    lv_color_t dotColor = fixed ? (hdop < 2.0f ? lv_color_hex(0x3FB950) : lv_color_hex(0xD29922)) : lv_color_hex(0xFF7B72);
    lv_obj_set_style_bg_color(gpsDot_, dotColor, 0);

    char buf[16];
    snprintf(buf, 16, fixed ? "%d" : "--", gps_? gps_->satellites() : -1);
    lv_label_set_text(satLabel_, buf);

    // Battery status
    if (ctrl_) {
        snprintf(buf, 16, "%d%%", ctrl_->getBatt());
        lv_label_set_text(battLabel_, buf);
    }

    snprintf(buf, 16, fixed ? "%.0f" : "--", gps_? gps_->speedMs() * 3.6f : 0);
    lv_label_set_text(speedLabel_, buf);

    snprintf(buf, 16, fixed ? "%dm" : "--", (int)(point? point->alt : 0));
    lv_label_set_text(altLabel_, buf);

    // Distance to marked home
    auto homepoint = map_.getHome();
    if (fixed && homepoint && point) {
        float d = homepoint.approxDistTo(*point);
        if (d > 1000.0f) snprintf(buf, 16, "%.1fkm", d / 1000.0f);
        else snprintf(buf, 16, "%.0fm", d);
        lv_label_set_text(distLabel_, buf);
    } else {
        lv_label_set_text(distLabel_, "H to set");
    }

    // Sidebar indicator dots
    for (uint8_t i = 0; i < (int)ViewID::COUNT; i++) {
        bool act = (uint8_t)ctrl_->currentView_ == i;
        lv_obj_set_style_bg_color(viewDots_[i], act ? lv_color_hex(COL_ACCENT) : lv_color_hex(COL_TEXT_DIM), 0);
    }

    // Recording indicator
    bool rec = ctrl_ && ctrl_->isRecording();
    lv_obj_set_style_bg_color(recDot_, rec ? lv_color_hex(0xFF3333) : lv_color_hex(0x333344), 0);
    lv_label_set_text(recLabel_, rec ? "REC" : "");
}

lv_obj_t* MapView::_makeLabel(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, const lv_font_t* font) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_pos(l, x, y);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(COL_TEXT), 0);
    return l;
}

lv_obj_t* MapView::_makeDot(lv_obj_t* parent, lv_coord_t x, lv_coord_t y) {
    lv_obj_t* d = lv_obj_create(parent);
    lv_obj_set_size(d, 8, 8);
    lv_obj_set_pos(d, x, y);
    lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(d, 0, 0);
    lv_obj_set_style_bg_color(d, lv_color_hex(COL_TEXT_DIM), 0);
    lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
    lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);
    return d;
}

void MapView::_makeDivider(lv_obj_t* parent, lv_coord_t y) {
    lv_obj_t* div = lv_obj_create(parent);
    lv_obj_set_size(div, SIDEBAR_W - 8, 1);
    lv_obj_set_pos(div, 4, y);
    lv_obj_set_style_bg_color(div, lv_color_hex(0x2a3040), 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);
}
