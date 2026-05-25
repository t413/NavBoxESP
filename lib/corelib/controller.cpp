#include "controller.h"
#include "MapView.h"
#include "FilesView.h"
#include <log.h>
#include <M5Cardputer.h>

Controller::Controller() {
}

void Controller::setup(lv_obj_t* parent) {
    gps_.begin(Serial1);

    // Initialize views
    views_[(int)ViewID::MAP] = new MapView();
    views_[(int)ViewID::TEST] = new TestView();
    views_[(int)ViewID::FILES] = (ViewBase*) new FilesView();

    for (int i = 0; i < (int)ViewID::COUNT; i++) {
        if (views_[i]) views_[i]->create(parent, this);
    }

    switchView(ViewID::MAP);
}

void Controller::iterate(uint32_t now) {
    // 1. Update GPS/State
    if (gps_.iterate(now)) {
        getMapView()->onGPSUpdate(&gps_);
        if (recordTrack_.isRecording()) {
            recordTrack_.addPoint(gps_.toPoint());
        }
    }

    // 2. Keyboard handling
    M5Cardputer.update();
    auto& kb = M5Cardputer.Keyboard;
    if (kb.isChange()) {
        if (kb.isKeyPressed(KEY_TAB)) {
            int next = ((int)currentView_ + 1) % (int)ViewID::COUNT;
            switchView((ViewID)next);
        } else if (kb.isKeyPressed(ctrlbtns::KEY_ESC)) {
            switchView(ViewID::MAP);
        } else { // Forward keys to active view
            ViewBase* active = views_[(int)currentView_];
            if (active) {
                // Normal character keys
                for (auto c : kb.keysState().word) {
                    active->onKey((uint8_t)c);
                }
                // Special function keys mapped to custom codes
                if (kb.isKeyPressed(KEY_ENTER)) active->onKey(ctrlbtns::KEY_RETURN);
            }
        }
    }

    // 3. Update all views
    for (uint8_t i = 0; i < (uint8_t)ViewID::COUNT; i++) {
        if (!views_[i]) continue;
        views_[i]->update(i == (uint8_t)currentView_);
    }
}

uint8_t Controller::getBatt() const {
    return M5.Power.getBatteryLevel();
}

bool Controller::toggleRecording() {
    if (recordTrack_.isRecording()) {
        recordTrack_.stopRecording();
        return false;
    } else {
        char path[64];
        snprintf(path, 64, "/tracks/%u.gpx", gps_.epoch());
        return recordTrack_.beginRecording(path);
    }
}

void Controller::switchView(ViewID id) {
    ViewBase* cur = views_[(int)currentView_];
    ViewBase* next = views_[(int)id];
    if (cur) cur->hide();
    if (next) next->show();
    currentView_ = id;
}

MapView* Controller::getMapView() {
    return (MapView*) views_[(int) ViewID::MAP];
}
