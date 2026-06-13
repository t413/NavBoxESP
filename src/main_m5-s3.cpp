#include <Arduino.h>
#include <M5Unified.h>
#include <M5Cardputer.h>
#include <SD.h>
#include <lvgl.h>
#include <navboxlib/log.h>
#include <basetilewriter.h>
#include "controller.h"
#include "version.h"

// LVGL display + touch buffers
lv_display_t * _disp = nullptr;
static lv_color_t* _buf1 = nullptr;
static lv_color_t* _buf2 = nullptr;

static void _setupLvgl(int dispW, int dispH);
void setupM5Input(Controller* c);

static constexpr int SD_SPI_SCK_PIN  = 40;
static constexpr int SD_SPI_MISO_PIN = 39;
static constexpr int SD_SPI_MOSI_PIN = 14;
static constexpr int SD_SPI_CS_PIN   = 12;

static Controller ctrl(GIT_VERSION);

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);  // landscape

    Serial.begin(115200);
    MAP_LOG("Starting up (free: %u)", ESP.getFreeHeap());

    _setupLvgl(M5.Lcd.width(), M5.Lcd.height());

    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        // g_state.sdMounted = true;
        // SD.mkdir(GPX_ROOT);
        MAP_LOG("[SD] Mounted. size %dm type %d", SD.cardSize()/1000000, SD.cardType());
        ensureBaseTilesOnSD(BASEDIR_TILES);
    } else {
        MAP_LOG("[SD] Not found — tile/GPX features disabled");
    }

    ctrl.setup(screen);
    ctrl.setupGPS(-1, -1, 115200, Serial1);
    setupM5Input(&ctrl);

    lv_task_handler();
    MAP_LOG("Setup complete (free %d) version %s", ESP.getFreeHeap(), GIT_VERSION);
}

void loop() {
    M5.update();
    uint32_t now = millis();
    ctrl.iterate(now);
    lv_timer_handler_run_in_period(10);
    yield();
}

static void _setupLvgl(int dispW, int dispH) {
    MAP_LOG("lvgl init start %dx%d (free: %u)", dispW, dispH, ESP.getFreeHeap());
    lv_init();
    lv_tick_set_cb([]() { return (uint32_t)millis(); });
    const int bufsize = dispW * 20 * sizeof(lv_color_t);
    _buf1 = (lv_color_t*)malloc(bufsize);
    _buf2 = (lv_color_t*)malloc(bufsize);
    MAP_LOG("lvgl init made bufs (free: %u)", ESP.getFreeHeap());

    _disp = lv_display_create(dispW, dispH);
    lv_display_set_buffers(_disp, _buf1, _buf2, bufsize, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(_disp, [](lv_display_t* disp, const lv_area_t* area, uint8_t* data) {
        uint32_t w = area->x2 - area->x1 + 1;
        uint32_t h = area->y2 - area->y1 + 1;
        M5.Display.startWrite();
        M5.Display.setAddrWindow(area->x1, area->y1, w, h);
        // M5Unified pushPixels accepts RGB565
        M5.Display.pushPixels((uint16_t*)data, w * h, true);
        M5.Display.endWrite();
        lv_display_flush_ready(disp);
    });
    lv_display_set_default(_disp);
    MAP_LOG("lvgl init done (free: %u)", ESP.getFreeHeap());
}


// ------------------------ //
// ---- Input handling ---- //
// ------------------------ //

class M5Keyboard : public InputBase {
public:
    bool begin(ControllerBase* ctrl) override {
        InputBase::begin(ctrl);
        M5Cardputer.begin();
        return true;
    }
    void end() override {}
    bool iterate(uint32_t now) override {
        if (!ctrl_) return false;
        M5Cardputer.update();
        auto& kb = M5Cardputer.Keyboard;
        if (kb.isChange()) {
            if (kb.isKeyPressed(KEY_TAB)) ctrl_->onKey('\t', now);
            if (kb.isKeyPressed(KEY_ENTER)) ctrl_->onKey(ctrlbtns::KEY_RETURN, now);
            if (kb.isKeyPressed(KEY_BACKSPACE)) ctrl_->onKey(ctrlbtns::KEY_DELETE, now);
            for (auto c : kb.keysState().word) { ctrl_->onKey(c, now); }
            return true;
        }
        return false;
    }
};

class M5Battery : public BattManagerBase {
public:
    void setup() override { M5.begin(); }
    float getMV() const override {
        return M5.Power.getBatteryVoltage() * 1000.0f;
    }
    float getMVtoPercent(uint16_t mv) const override {
        return M5.Power.getBatteryLevel(); // M5 API provides percentage directly
    }
};

static M5Keyboard m5_keyboard;
static M5Battery m5_batt;

void setupM5Input(Controller* c) {
    m5_keyboard.begin(c);
    c->addInput(&m5_keyboard);
    m5_batt.setup();
    c->setBattMgr(&m5_batt);
}
