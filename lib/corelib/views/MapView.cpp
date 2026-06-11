#include "MapView.h"
#include <navboxlib/log.h>
#include <navboxlib/MapLayer.h>
#include "../controller.h"
#include "../GpsManager.h"
#include <lvgl.h>

// Local constants to replace Include/Config.h
static constexpr uint32_t COL_SIDEBAR_BG = 0x161B22; //161B22
static constexpr uint32_t COL_TEXT       = 0xE6EDF3; //E6EDF3
static constexpr uint32_t COL_TEXT_DIM   = 0x8B949E; //ccd4dc
static constexpr uint32_t COL_ACCENT     = 0x58A6FF; //58A6FF

void MapView::loadSettings(SettingsManager& mgr) {
    auto group = mgr.group("map");
    group.addFn("Home Position", [this]() -> SetValue {
        return geoPointToStr(markerLayer().get(homeMarkerId_).pos);
    }, [this](SetValue v) {
        markerLayer().updatePoint(homeMarkerId_, parseGeoPoint(v));
    }).setLate(true);
    group.addFn("Default Start", [this]() -> SetValue {
        return geoPointToStr(getMap().getCenter());
    }, [this](SetValue v) {
        getMap().setCenter(parseGeoPoint(v));
    });
}

void MapView::create(lv_obj_t* parent, ControllerBase* ctrl) {
    root_ = lv_obj_create(parent);
    ctrl_ = (Controller*)ctrl;
    const auto [dispW, dispH] = ctrl->getDispSize();
    int sidebarW = (dispW > 240) ? 70 : 60;

    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);

    // Initialize Map on the left side
    map_.begin(root_, dispW - sidebarW, dispH, BASEDIR_TILES "/%d/%d/%d.png");
    map_.setXY(0, 0);
    map_.setSmartInvert(true);

    _createSidebar(root_);

    recTrackLayer_ = new TrackLayer(&map_, map_.colAccent_, &ctrl_->recordTrack_);
    viewTrackLayer_ = new TrackLayer(&map_, 0xFF7B72, &ctrl_->viewTrack_);
    map_.addLayer(recTrackLayer_);
    map_.addLayer(viewTrackLayer_);

    auto& lyr = markerLayer();
    homeMarkerId_ = lyr.add(Marker({}, map_.homesize_, map_.colHome_, 'H'));
    dotMarkerId_ = lyr.add(Marker({}, map_.dotsize_, map_.colAccent_)); //second to be on top
    MAP_LOG("map created [%dx%d], %d layers, crop %d", map_.getWidth(), map_.getHeight(), (int)map_.getLayers().size(), map_.cropmode_);

    _updateSidebar();
    map_.setZoom(12);
    map_.iterate(millis(), true); //block for initial tiles load
}

void MapView::hide() { lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN); isActive_ = false; }
void MapView::show() {
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    isActive_ = true;
    _updateSidebar();
    map_._updateLayers();
}

MarkerLayer& MapView::markerLayer() {
    return *(map_.getMarkerLayer());
}

void MapView::iterate(uint32_t now, bool inview) {
    isActive_ = inview;
    //map_ updates handled by key events or GPS point updates
    if (inview)
        map_.iterate(now);
}

static constexpr int16_t CROP_RECENTER_DIST = 14;
static constexpr uint32_t CROP_FORCE_REDRAW_MS = 2000;

void MapView::onGPSUpdate(GpsManager* gps) {
    if (!gps) return;
    if (!gps_) gps_ = gps;
    auto point = gps->toPoint();
    _updateSidebar(&point);
    markerLayer().updatePoint(dotMarkerId_, point);
    if (!isActive_) {
        MAP_LOG("mapview:redraw skip, inactive");
        return;
    }

    if (followMode_ && gps->hasFix()) {
        bool needsCenter = !map_.cropmode_; //cropmode needs infrequent centering
        if (map_.cropmode_) {
            uint32_t now = millis();
            if ((now - lastForceRedraw_) > CROP_FORCE_REDRAW_MS) { //limit tile loading
                auto pos = map_.getMarkerPx(dotMarkerId_);
                int16_t px = (pos.x != -1)? map_.getPxDistToCenter(pos) : 1000;
                if (px >= CROP_RECENTER_DIST) {
                    MAP_LOG("mapview:redraw %d px", px);
                    needsCenter = true;
                    lastForceRedraw_ = now;
                }
            }
        }

        if (needsCenter) {
            map_.setCenter(point);
        } else map_._updateLayers(); //just update markers/track
    }
}

bool MapView::onKey(uint8_t key, uint32_t now) {
    if (key == ctrlbtns::KEY_ARROW_UP) { map_.panPx(0, -20); followMode_ = false;
    } else if (key == ctrlbtns::KEY_ARROW_DOWN) { map_.panPx(0, 20);  followMode_ = false;
    } else if (key == ctrlbtns::KEY_ARROW_LEFT) { map_.panPx(-20, 0); followMode_ = false;
    } else if (key == ctrlbtns::KEY_ARROW_RIGHT) { map_.panPx(20, 0);  followMode_ = false;
    } else if (key == '[' || key == 'i') { map_.setZoom(map_.zoomtotal() - 1);
    } else if (key == ']' || key == 'o') { map_.setZoom(map_.zoomtotal() + 1);
    } else if (key == 'b') { map_.setSmartInvert(!map_.getInverted());
    } else if (key == 'f') {
        followMode_ = true;
        if (gps_ && gps_->hasFix()) map_.setCenter(gps_->toPoint());
    } else if (key == 'h') {
        markerLayer().updatePoint(homeMarkerId_, getMap().getCenter());
        map_._updateLayers();
    } else if (key == 'r') {
        if (ctrl_) ctrl_->toggleRecording();
    } else { return false; }
    _updateSidebar();
    return true;
}

bool MapView::onTouch(const BaseTouchPoint& pt, uint32_t now) {
    if (pt.pressed) {
        //TODO check for click position, allow other elements to be clicked
        if (!isPressed_) {
            MAP_LOG("mapv: touch down %d,%d", pt.x, pt.y);
            isPressed_ = true;
        } else {
            map_.panPx(lastTouch_.x - pt.x, lastTouch_.y - pt.y);
            followMode_ = false;
        }
        lastTouch_ = pt;
    } else {
        MAP_LOG("mapv: touch end %d,%d", pt.x, pt.y);
        isPressed_ = false;
    }
    return true;
}

bool MapView::onScroll(const TrackballDelta& delta, uint32_t now) {
    if (delta.dx != 0 || delta.dy != 0) { //move map!
        if (delta.pressed && delta.dy == 0) {
            return false; //let controller handle click
        } else if (delta.pressed) {
            map_.setZoom(map_.zoomtotal() + delta.dy);
        } else {
            map_.panPx(delta.dx * 4, delta.dy * 4);
            followMode_ = false;
        }
    }
    return true;
}


bool MapView::handleBack() {
    if (!followMode_ && gps_ && gps_->hasFix()) {
        setCenter(gps_->toPoint());
        return true;
    } else if (ctrl_ && ctrl_->viewTrack_.size()) {
        setCenter(ctrl_->viewTrack_.calcCenter());
        return true;
    }
    return false;
}

void MapView::setCenter(const GeoPoint &p) {
    // If centering on current GPS location, enable follow mode
    if (gps_ && gps_->hasFix()) {
        followMode_ = (p.approxDistTo(gps_->toPoint()) < 1.0);
    }
    map_.setCenter(p);
}

void MapView::_createSidebar(lv_obj_t* parent) {
    const auto [dispW, dispH] = ctrl_->getDispSize();
    int sidebarW = dispW - map_.getWidth();

    sidebar_ = lv_obj_create(parent);
    lv_obj_set_size(sidebar_, sidebarW, dispH);
    lv_obj_set_pos(sidebar_, map_.getWidth(), 0);
    lv_obj_set_style_bg_color(sidebar_, lv_color_hex(COL_SIDEBAR_BG), 0);
    lv_obj_set_style_border_side(sidebar_, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(sidebar_, 1, 0);
    lv_obj_set_style_pad_all(sidebar_, 0, 0);
    lv_obj_set_scrollbar_mode(sidebar_, LV_SCROLLBAR_MODE_OFF);

    gpsDot_ = _makeDot(sidebar_, 4, 6);
    satLabel_ = _makeLabel(sidebar_, 14, 2, &lv_font_montserrat_12);
    lv_obj_set_style_text_color(satLabel_, lv_color_hex(COL_TEXT), 0);

    battLabel_ = _makeLabel(sidebar_, sidebarW - 24, 2, &lv_font_montserrat_12);
    lv_obj_set_style_text_color(battLabel_, lv_color_hex(COL_TEXT), 0);

    speedLabel_ = _makeLabel(sidebar_, 2, 18, &lv_font_montserrat_14);
    lv_obj_set_width(speedLabel_, sidebarW - 4);
    lv_obj_set_style_text_color(speedLabel_, lv_color_hex(COL_ACCENT), 0);
    lv_obj_set_style_text_align(speedLabel_, LV_TEXT_ALIGN_CENTER, 0);

    _makeDivider(sidebar_, 44);
    altLabel_ = _makeLabel(sidebar_, 2, 60, &lv_font_montserrat_12);
    lv_obj_set_width(altLabel_, sidebarW - 4);
    lv_obj_set_style_text_align(altLabel_, LV_TEXT_ALIGN_CENTER, 0);

    _makeDivider(sidebar_, 74);
    distLabel_ = _makeLabel(sidebar_, 2, 90, &lv_font_montserrat_12);
    lv_obj_set_width(distLabel_, sidebarW - 4);
    lv_obj_set_style_text_align(distLabel_, LV_TEXT_ALIGN_CENTER, 0);

    _makeDivider(sidebar_, 104);
    recDot_ = _makeDot(sidebar_, 4, 112);
    recLabel_ = _makeLabel(sidebar_, 14, 110, &lv_font_montserrat_10);

    for (int i = 0; i < (int)ViewID::COUNT; i++) {
        viewDots_[i] = _makeDot(sidebar_, 4 + i * 10, dispH - 10);
        lv_obj_set_size(viewDots_[i], 6, 6);
    }
}

void MapView::_updateSidebar(const TrackPoint* point) {
    lv_obj_set_style_border_color(sidebar_, lv_color_hex(map_.getInverted()? sidebarBg_ : 0x2a3040), 0); // #2a3040
    bool fixed = gps_ && gps_->hasFix();
    if (!point && fixed) point = &gps_->toPoint();
    float hdop = gps_ ? gps_->hdop() : 99.9f;
    lv_color_t dotColor = fixed ? (hdop < 2.0f ? lv_color_hex(0x3FB950) : lv_color_hex(0xD29922)) : lv_color_hex(0xFF7B72);
    lv_obj_set_style_bg_color(gpsDot_, dotColor, 0);

    char buf[16];
    snprintf(buf, 16, fixed ? "%d" : "--", fixed? gps_->satellites() : 0);
    lv_label_set_text(satLabel_, buf);

    if (ctrl_) {
        snprintf(buf, 16, "%d%%", (uint8_t) ctrl_->getBatt());
        lv_label_set_text(battLabel_, buf);
    }

    snprintf(buf, 16, fixed ? "%.0f" : "--", fixed? gps_->speedMs() * 3.6f : 0.0f);
    lv_label_set_text(speedLabel_, buf);

    snprintf(buf, 16, fixed ? "%dm" : "--", (int)(fixed? point->alt : 0));
    lv_label_set_text(altLabel_, buf);

    // Distance to marked home
    auto homepoint = homeMarkerId_? markerLayer().get(homeMarkerId_).pos : GeoPoint();
    if (homepoint) {
        auto topoint = (point && fixed)? *point : getMap().getCenter();
        float d = homepoint.approxDistTo(topoint);
        if (d > 1000.0f) snprintf(buf, 16, "%.1fkm", d / 1000.0f);
        else snprintf(buf, 16, "%.0fm", d);
        lv_label_set_text(distLabel_, buf);
    } else {
        lv_label_set_text(distLabel_, "H to set");
    }

    // Sidebar indicator dots
    for (uint8_t i = 0; i < (int)ViewID::COUNT; i++) {
        bool act = ctrl_ && (uint8_t)ctrl_->currentView_ == i;
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
    lv_coord_t w = lv_obj_get_width(parent);
    lv_obj_t* div = lv_obj_create(parent);
    lv_obj_set_size(div, w - 8, 1);
    lv_obj_set_pos(div, 4, y);
    lv_obj_set_style_bg_color(div, lv_color_hex(0x2a3040), 0);
    lv_obj_set_style_border_width(div, 0, 0);
    lv_obj_clear_flag(div, LV_OBJ_FLAG_SCROLLABLE);
}

GeoPoint parseGeoPoint(const String& text) {
    int comma = text.indexOf(',');
    if (comma < 0) return GeoPoint();
    String latStr = text.substring(0, comma);
    String lonStr = text.substring(comma + 1);
    if (latStr.length() == 0 || lonStr.length() == 0) return false;
    double lat = latStr.toFloat();
    double lon = lonStr.toFloat();
    return GeoPoint(lat, lon);
}
String geoPointToStr(const GeoPoint& p) {
    if (!p) return "";
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f,%.6f", p.lat(), p.lon());
    return String(buf);
}
