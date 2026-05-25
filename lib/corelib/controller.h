#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "GpsManager.h"
#include "TrackLog.h"
#include "View.h"

namespace ctrlbtns {
    // keyboard buttons
    static constexpr char KEY_ESC = '`';
    static constexpr char KEY_RETURN = '\n';
    static constexpr char KEY_ARROW_UP = ';';
    static constexpr char KEY_ARROW_DOWN = '.';
    static constexpr char KEY_ARROW_LEFT = ',';
    static constexpr char KEY_ARROW_RIGHT = '/';
}
class MapView;

class Controller {
public:
    Controller();

    void setup(lv_obj_t* parent);
    void iterate(uint32_t now);
    void switchView(ViewID id);

    bool toggleRecording();
    bool isRecording() const { return recordTrack_.isRecording(); }

    uint8_t getBatt() const; /// battery percentage

private:
    MapView* getMapView();

public:
    GpsManager gps_;
    TrackLog recordTrack_;
    TrackLog viewTrack_;
    ViewID currentView_ = ViewID::MAP;
    ViewBase* views_[(int)ViewID::COUNT] = {};
};
