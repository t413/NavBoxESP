#include "controller.h"
#include "views/AboutView.h"
#include "views/MapView.h"
#include <log.h>
#include <M5Cardputer.h>

Controller::Controller(const char* v) : version_(v), recordTrack_(BASEDIR_TRACKS_REC) {}

void Controller::setup(lv_obj_t* parent) {
    parent_ = parent;
    gps_.begin(Serial1);

    // Initialize views
    views_[(int)ViewID::MAP] = new MapView();
    views_[(int)ViewID::ABOUT] = new AboutView();

    for (int i = 0; i < (int)ViewID::COUNT; i++) {
        if (views_[i]) views_[i]->create(parent, this);
    }
    switchView(ViewID::MAP);
    lastActivityMs_ = millis();
    M5.Display.setBrightness(screenBrightness_);
}

void Controller::iterate(uint32_t now) {
    // 1. Update GPS/State
    if (gps_.iterate(now)) {
        getMapView()->onGPSUpdate(&gps_);
        if (recordTrack_.isRecording()) {
            recordTrack_.addPoint(gps_.toPoint());
        }
    }
    _processKeys(now);
    _updateDimming(now);

    // 3. Update all views
    for (uint8_t i = 0; i < (uint8_t)ViewID::COUNT; i++) {
        if (!views_[i]) continue;
        views_[i]->iterate(i == (uint8_t)currentView_);
    }
}

void Controller::_processKeys(uint32_t now) {
    M5Cardputer.update();
    auto& kb = M5Cardputer.Keyboard;
    if (kb.isChange()) {
        ViewBase* active = views_[(uint8_t)currentView_];
        if (kb.isKeyPressed(KEY_TAB)) {
            int next = ((int)currentView_ + 1) % (int)ViewID::COUNT;
            switchView((ViewID)next);
        } else if (kb.isKeyPressed(ctrlbtns::KEY_ESC)) {
            if (active && !active->handleBack())
                switchView(ViewID::MAP);
        } else if (active) { // Forward keys to active view
            // Normal character keys
            for (auto c : kb.keysState().word) {
                active->onKey((uint8_t)c);
            }
            // Special function keys mapped to custom codes
            if (kb.isKeyPressed(KEY_ENTER)) active->onKey(ctrlbtns::KEY_RETURN);
        }
        lastActivityMs_ = now;
        if (dimmed_ || sleeping_) {
            dimmed_ = sleeping_ = false;
            M5.Display.wakeup();
            M5.Display.setBrightness(screenBrightness_);
        }
    }
}

void Controller::_updateDimming(uint32_t now) {
    if (screenDimSec_ == 0) return; // "never" mode
    uint32_t idle = (now - lastActivityMs_) / 1000;
    if (!dimmed_ && idle >= (screenDimSec_ - 5)) {
        dimmed_ = true;
        M5.Display.setBrightness(10); // very dim
    }
    if (idle >= screenDimSec_ && !sleeping_) {
        MAP_LOG("Display sleep after %us idle", idle);
        M5.Display.sleep();
        sleeping_ = true;
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
        return recordTrack_.beginRecording(gps_.epoch());
    }
}

bool Controller::loadTrack(const char* path) {
    if (viewTrack_.load(path)) {
        auto m = getMapView();
        auto center = viewTrack_.calcCenter();
        if (m && center)
            m->setCenter(center);
        switchView(ViewID::MAP);
        return true;
    }
    return false;
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
