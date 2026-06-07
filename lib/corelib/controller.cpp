#include "controller.h"
#include "views/AboutView.h"
#include "views/MapView.h"
#include "views/SettingsView.h"
#include <lvgl.h>
#include <navboxlib/log.h>
#ifdef USE_M5
#include <M5Cardputer.h>
#elif defined(USE_LGFX)
#include <LovyanGFX.hpp>
#endif

Controller::Controller(const char* v) : version_(v), recordTrack_(BASEDIR_TRACKS_REC) {}

static BaseTouchPoint g_touch_data = {};
static bool g_touch_ready = false;

void Controller::setup(lv_obj_t* parent) {
    parent_ = parent;
    // Full-screen overlay container for modals (sits above views, below nothing)
    overlayRoot_ = lv_obj_create(parent);
    lv_obj_set_size(overlayRoot_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(overlayRoot_, 0, 0);
    lv_obj_set_style_border_width(overlayRoot_, 0, 0);
    lv_obj_set_style_pad_all(overlayRoot_, 0, 0);
    lv_obj_set_scrollbar_mode(overlayRoot_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(overlayRoot_, LV_OBJ_FLAG_HIDDEN); // hidden when no modals

    // Initialize views
    views_[(int)ViewID::MAP] = new MapView();
    views_[(int)ViewID::ABOUT] = new AboutView();
    views_[(int)ViewID::SETTINGS] = new SettingsView();

    // first setup what settings exist
    loadSettings(settingsManager_);
    for (int i = 0; i < (int)ViewID::COUNT; i++) {
        if (views_[i]) views_[i]->loadSettings(settingsManager_);
    }
    settingsManager_.load(false); //now load from file into position

    for (int i = 0; i < (int)ViewID::COUNT; i++) { //now start views
        if (views_[i]) views_[i]->create(parent, this);
    }
    settingsManager_.load(true); //now load late settings
    switchView(ViewID::MAP);
    lastActivityMs_ = millis();
    setBrightness(screenBrightness_);
}

#ifdef USE_LGFX
void Controller::setupLgfx(lgfx::LGFX_Device& device) {
    lgfxDevice_ = &device;
}
#endif

void Controller::setupGPS(int rx, int tx, uint32_t baud, HardwareSerial& uart) {
    gps_.begin(rx, tx, baud, uart);
}

void Controller::loadSettings(SettingsManager& mgr) {
    auto group = mgr.group("basic");
    group.add("Brightness", &screenBrightness_, (uint8_t) 20, (uint8_t) 255).setAdjBump(20)
        .onChange([this](){ setBrightness(screenBrightness_); });
    group.add("Dim Time", &screenDimSec_, 0, 600).setAdjBump(10);
}

void Controller::setOverlay(ViewBase* overlay) {
    if (overlay) {
        lv_obj_clear_flag(overlayRoot_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(overlayRoot_); // Bring overlay to front so it renders above views
    } else {
        lv_obj_add_flag(overlayRoot_, LV_OBJ_FLAG_HIDDEN);
        overlay_->hide();
        delete overlay_;
    }
    overlay_ = overlay;
}

void Controller::iterate(uint32_t now) {
    for (auto input : inputs_) input->iterate(now);

    // 1. Update GPS/State
    if (gps_.iterate(now)) {
        getMapView()->onGPSUpdate(&gps_);
        if (recordTrack_.isRecording()) {
            recordTrack_.addPoint(gps_.toPoint());
            if ((recordTrack_.recordedPoints_ % 20) == 0)
                getMapView()->getMap()._updateLayers(); //periodically update
        }
    }
    _updateDimming(now);

    // 3. Update all views
    for (uint8_t i = 0; i < (uint8_t)ViewID::COUNT; i++) {
        if (!views_[i]) continue;
        views_[i]->iterate(i == (uint8_t)currentView_ && !sleeping_);
    }
}

bool Controller::onKey(uint8_t key, uint32_t now) {
    MAP_LOG("Input KEY: 0x%02X ('%c')", key, (key >= 32 && key < 127) ? (char)key : '?');
    if (wakeup(now)) return true; //consume input
    auto cv = getCurrentView();

    if (cv) { // Route to active view
        if (cv->onKey(key, now))
            return true;
    }
    if (key == ctrlbtns::KEY_ESC) { // Global shortcuts
        switchView(ViewID::MAP);  // Always can escape to map
    } else if (key == 'p') {
        switchView((ViewID)(((int)currentView_ + 1) % (int)ViewID::COUNT));
    }
    return false;
}

bool Controller::onTouch(const BaseTouchPoint& tp, uint32_t now) {
    MAP_LOG("Input TOUCH: x=%d y=%d press=%d", tp.x, tp.y, tp.pressed);
    if (wakeup(now)) return true; //consume input
    auto cv = getCurrentView();

    if (cv) { // Route to active view
        if (cv->onTouch(tp, now))
            return true;
    }

    // Let LVGL handle it (it has a touch input device driver)
    lv_indev_t* indev = lv_indev_get_next(nullptr);
    while (indev) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            g_touch_data = tp;
            g_touch_ready = true;
            break;
        }
        indev = lv_indev_get_next(indev);
    }
    return false;
}

bool Controller::onScroll(const TrackballDelta& delta, uint32_t now) {
    MAP_LOG("Input SCROLL: dx=%d dy=%d press=%d", delta.dx, delta.dy, delta.pressed);
    if (wakeup(now)) return true; //consume input
    auto cv = getCurrentView();
    if (cv) { // Route to active view
        if (cv->onScroll(delta, now))
            return true;
    }
    //TODO temporary: send arrow keys to layout
    if (delta.dy != 0) {
        return cv->onKey(delta.dy > 0 ? ctrlbtns::KEY_ARROW_UP : ctrlbtns::KEY_ARROW_DOWN, now);
    } else if (delta.dx != 0) {
        return cv->onKey(delta.dx > 0 ? ctrlbtns::KEY_ARROW_RIGHT : ctrlbtns::KEY_ARROW_LEFT, now);
    } else if (delta.pressed && !lastBtn_) {
        auto ret = cv->onKey(ctrlbtns::KEY_RETURN, now);
        lastBtn_ = delta.pressed;
        return ret;
    }

    if (delta.pressed && !lastBtn_) {
        int next = ((int)currentView_ + 1) % (int)ViewID::COUNT;
        switchView((ViewID)next);
    }
    lastBtn_ = delta.pressed;
    return false;
}

void Controller::setBrightness(uint8_t b) {
#ifdef USE_M5
    M5.Display.setBrightness(b);
    if (b <= 1) M5.Display.sleep();
    else if (dimmed_ || sleeping_) M5.Display.wakeup();
#elif defined(USE_LGFX)
    if (lgfxDevice_) {
        lgfxDevice_->setBrightness(b);
        if (b <= 1) lgfxDevice_->sleep();
        else if (dimmed_ || sleeping_) lgfxDevice_->wakeup();
    }
#endif
    for (auto input : inputs_) input->setBrightness(b);
}

std::pair<uint16_t, uint16_t> Controller::getDispSize() const {
#ifdef USE_M5
    return std::make_pair(M5.Lcd.width(), M5.Lcd.height());
#elif defined(USE_LGFX)
    return std::make_pair(lgfxDevice_->width(), lgfxDevice_->height());
#else //from lvgl
    auto def = lv_disp_get_default();
    return std::make_pair(lv_disp_get_hor_res(def), lv_disp_get_ver_res(def));
#endif
}

void Controller::_updateDimming(uint32_t now) {
    if (screenDimSec_ == 0) return; // "never" mode
    uint32_t idle = (now - lastActivityMs_) / 1000;
    if (!dimmed_ && idle >= (screenDimSec_ - 5)) {
        MAP_LOG("Display dim after %us idle", idle);
        dimmed_ = true;
        setBrightness(10); // very dim
    }
    if (idle >= screenDimSec_ && !sleeping_) {
        MAP_LOG("Display sleep after %us idle", idle);
        setBrightness(0); //panel off
        sleeping_ = true;
        doLightSleep();
    }
}

bool Controller::wakeup(uint32_t now) {
    lastActivityMs_ = now;
    if (dimmed_ || sleeping_) {
        setBrightness(screenBrightness_);
        dimmed_ = sleeping_ = false;
        getMapView()->getMap()._updateLayers();
        return true;
    }
    return false;
}

void Controller::doLightSleep() {
    while (sleeping_) {
        MAP_LOG("light sleeping btn %d", digitalRead(GPIO_NUM_0));
        for (auto input : inputs_) input->onSleep(true);
        auto start = millis();
        esp_sleep_enable_uart_wakeup(1);  // UART1 = Serial1
        gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_LOW_LEVEL);
        esp_sleep_enable_gpio_wakeup();
        esp_light_sleep_start();
        // we get to here if woken by gpio interrupt or uart
        auto now = millis();
        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        MAP_LOG("light sleep wake after %dms, cause %d btn %d", now - start, cause, digitalRead(GPIO_NUM_0));
        if (cause == ESP_SLEEP_WAKEUP_GPIO) {
            wakeup(now); break; //we're up!
        } else {
            for (uint8_t i = 0; i < 10; i++) {
                iterate(now); //checks keys, reads uart, allows wakeup
                delay(1);
            }
        }
    }
    MAP_LOG("light sleep done, re-enabling %d inputs", (int)inputs_.size());
    for (auto input : inputs_) input->onSleep(false);
}

uint8_t Controller::getBatt() const {
#ifdef USE_M5
    return M5.Power.getBatteryLevel();
#else
    return 0; // TODO for t-deck
#endif
}

bool Controller::toggleRecording() {
    if (recordTrack_.isRecording()) {
        recordTrack_.stopRecording();
        return false;
    } else {
        return recordTrack_.recordResume(gps_.epoch());
    }
}

bool Controller::loadTrack(const char* path, TrackLog* to) {
    if (!to) to = &viewTrack_;
    if (to->load(path)) {
        auto m = getMapView();
        auto center = to->calcCenter();
        if (m && center)
            m->setCenter(center);
        switchView(ViewID::MAP);
        return true;
    }
    return false;
}

ViewBase* Controller::getCurrentView() const {
    return overlay_? overlay_ : views_[(int)currentView_];
}

void Controller::switchView(ViewID id) {
    ViewBase* cur = getCurrentView();
    if (cur == overlay_) setOverlay(nullptr);
    ViewBase* next = views_[(int)id];
    if (cur) cur->hide();
    if (next) next->show();
    currentView_ = id;
}

MapView* Controller::getMapView() {
    return (MapView*) views_[(int) ViewID::MAP];
}
