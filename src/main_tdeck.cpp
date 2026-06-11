#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <navboxlib/log.h>
#include <basetilewriter.h>
#include "controller.h"
#include "version.h"
#include <TouchDrv.hpp>

// Default pin definitions (can be overridden by build_flags)
constexpr uint8_t PIN_POWER_ON  = 10;
constexpr uint8_t SHARED_SCK    = 40;
constexpr uint8_t SHARED_MISO   = 38;
constexpr uint8_t SHARED_MOSI   = 41;
constexpr uint8_t SDCARD_CS     = 39;
constexpr uint8_t TFT_CS        = 12;
constexpr uint8_t TFT_BL        = 42;
constexpr uint8_t TFT_DC        = 11;
constexpr uint16_t TFT_WIDTH    = 240; //rotated later
constexpr uint16_t TFT_HEIGHT   = 320;
constexpr uint8_t ROTATION      = 1;
constexpr uint8_t SHARED_I2C_SDA    = 18;
constexpr uint8_t SHARED_I2C_SCL    = 8;
constexpr uint8_t KB_I2S_ADDR       = 0x55;
constexpr uint8_t TRACKBALL_SEL     = 0;
constexpr uint8_t TRACKBALL_UP      = 3;
constexpr uint8_t TRACKBALL_DOWN    = 15;
constexpr uint8_t TRACKBALL_LEFT    = 2;
constexpr uint8_t TRACKBALL_RIGHT   = 1;
constexpr uint8_t TOUCH_INT         = 16;
constexpr uint8_t TOUCH_ADDR_A      = 0x5D; //can be either based on int pin state
constexpr uint8_t TOUCH_ADDR_B      = 0x14;
constexpr uint8_t GPS_RX = 44;
constexpr uint8_t GPS_TX = 43;
constexpr uint8_t SPIBUS_HOST = FSPI;
constexpr uint8_t PIN_BAT_ADC = 4;
constexpr float BAT_ADC_MULT = (2.0f * 3.3f * 1000.0f);

SPIClass spibus(SPIBUS_HOST);

// LVGL display + touch buffer
lv_display_t * _disp = nullptr;
static lv_color_t* _buf1 = nullptr;
static lv_color_t* _buf2 = nullptr;
static Controller ctrl(GIT_VERSION);

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel;
    lgfx::Bus_SPI      _bus;
    lgfx::Light_PWM    _light;
public:
    LGFX() {
        auto bcfg = _bus.config();
        bcfg.spi_host = SPI2_HOST;
        bcfg.spi_mode = 0;
        bcfg.freq_write = 40000000;
        bcfg.freq_read  = 16000000;
        bcfg.pin_sclk = SHARED_SCK;
        bcfg.pin_mosi = SHARED_MOSI;
        bcfg.pin_miso = SHARED_MISO;
        bcfg.pin_miso = SHARED_MISO;
        bcfg.pin_dc   = TFT_DC;
        bcfg.dma_channel = SPI_DMA_CH_AUTO;
        _bus.config(bcfg);
        _panel.setBus(&_bus);
        auto pcfg = _panel.config();
        pcfg.pin_cs = TFT_CS;
        pcfg.panel_width  = pcfg.memory_width  = TFT_WIDTH;
        pcfg.panel_height = pcfg.memory_height = TFT_HEIGHT;
        pcfg.offset_rotation = ROTATION;
        pcfg.readable = true;
        pcfg.invert = true;
        _panel.config(pcfg);
        setPanel(&_panel);
        auto lcfg = _light.config();
        lcfg.pin_bl = TFT_BL;
        lcfg.invert = false;
        lcfg.freq   = 44100;
        lcfg.pwm_channel = 0;
        _light.config(lcfg);
        _panel.setLight(&_light);
    }
};

static LGFX device;

static void _setupLvgl(int dispW, int dispH);
void setupTDeckInput(Controller*);
uint16_t readBatteryMV();

void setup() {
    Serial.begin(115200);
    MAP_LOG("T-Deck startup (free: %u)", ESP.getFreeHeap());

    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    auto iret = device.init();
    // device.setRotation(ROTATION);
    device.fillScreen(0);
    device.setTextColor(0x58A6FF); // COL_ACCENT
    device.setTextDatum(middle_center);
    device.drawString("NavBox", device.width() / 2, device.height() / 2 - 10, &fonts::FreeSansBold12pt7b);
    device.drawString("by t413", device.width() / 2, device.height() / 2 + 10, &fonts::FreeSerifItalic12pt7b);

    _setupLvgl(device.width(), device.height());
    MAP_LOG("Display Init %d: %dx%d, colorDepth=%d, r %d", iret, device.width(), device.height(),  device.getColorDepth(), device.getRotation());

    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    ctrl.setupLgfx(device);

    // init SPI for SD card (use defined pins)
    spibus.begin(SHARED_SCK, SHARED_MISO, SHARED_MOSI, SDCARD_CS);
    if (SD.begin(SDCARD_CS, spibus, 25000000)) {
        MAP_LOG("[SD] Mounted. size %dm type %d", SD.cardSize()/1000000, SD.cardType());
        ensureBaseTilesOnSD(BASEDIR_TILES);
    } else {
        MAP_LOG("[SD] Not found — tile/GPX features disabled");
    }
    ctrl.setup(screen);
    ctrl.setupGPS(GPS_RX, GPS_TX, 38400, Serial1);
    setupTDeckInput(&ctrl);

    lv_task_handler();
    MAP_LOG("T-Deck setup complete (free %d) version %s", ESP.getFreeHeap(), GIT_VERSION);
}

void loop() {
    uint32_t now = lgfx::millis();
    ctrl.iterate(now);
    lv_timer_handler_run_in_period(10);
    yield();
}

static void _setupLvgl(int dispW, int dispH) {
    MAP_LOG("lvgl init start %dx%d (free: %u)", dispW, dispH, ESP.getFreeHeap());
    lv_init();
    lv_tick_set_cb([]() { return (uint32_t)millis(); });

    _disp = lv_display_create(dispW, dispH);

    const int bufsize = sizeof(lv_color_t) * dispW * dispH;
    _buf1 = (lv_color_t*)malloc(bufsize);
    // _buf2 = (lv_color_t*)malloc(bufsizes);
    MAP_LOG("lvgl init made bufs [%d] (free: %u)", bufsize, ESP.getFreeHeap());

    lv_display_set_buffers(_disp, _buf1, _buf2, bufsize, LV_DISPLAY_RENDER_MODE_FULL);

    lv_display_set_flush_cb(_disp, [](lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
        uint32_t w = area->x2 - area->x1 + 1;
        uint32_t h = area->y2 - area->y1 + 1;
        device.startWrite();
        device.setAddrWindow(area->x1, area->y1, w, h);
        device.writePixels((lgfx::rgb565_t*)px_map, w * h);
        device.endWrite();
        lv_display_flush_ready(disp);
    });

    lv_display_set_default(_disp);
    MAP_LOG("lvgl init done, size %d (free: %u)", dispW * dispH, ESP.getFreeHeap());
}

// ------------------------ //
// ---- Input handling ---- //
// ------------------------ //


class TDeckKeyboard : public InputBase {
    bool keyboard_present_ = false;
public:
    bool begin(ControllerBase* ctrl) override {
        InputBase::begin(ctrl);
        Wire.beginTransmission(KB_I2S_ADDR);
        keyboard_present_ = (Wire.endTransmission(true) == 0); // 0 is success
        MAP_LOG("I2C keyboard at 0x%02X -> %d after", KB_I2S_ADDR, keyboard_present_);
        return keyboard_present_;
    }
    void end() override { } // Could power down keyboard here if needed
    bool iterate(uint32_t now) override {
        if (!keyboard_present_) return false;
        Wire.beginTransmission(KB_I2S_ADDR);
        if (Wire.endTransmission() == 0) {
            if (Wire.requestFrom((uint8_t)KB_I2S_ADDR, (uint8_t)1) >= 1) {
                if (Wire.available()) {
                    auto key = (char)Wire.read();
                    if (key != 0) ctrl_->onKey(key, now);
                }
            }
        }
        return true;
    }
    bool setBrightness(uint8_t brightval) override {
        // if (!keyboard_present_) return false;
        Wire.beginTransmission(KB_I2S_ADDR);
        auto r1 = Wire.write(0x01); //brightness command
        auto r2 = Wire.write(brightval);
        auto r3 = Wire.endTransmission();
        MAP_LOG("keyboard:bright(%d) [%d,%d,%d]", brightval, r1, r2, r3);
        return true;
    }
    void onSleep(bool sleeping) override { keyboard_present_ = !sleeping; } //disables kb
};

class TDeckTouch : public InputBase {
    TouchDrvGT911 touch_;
    bool touch_present_ = false;
    bool lastPressed_ = false;
    uint32_t lastPressChange_ = 0;
    const int pressDebounceMs_ = 40;
public:
    bool begin(ControllerBase* ctrl) override {
        InputBase::begin(ctrl);
        MAP_LOG("TDeckTouch", "Starting GT911 touchscreen at 0x%02X, int %d", TOUCH_ADDR_A, TOUCH_INT);
        pinMode(TOUCH_INT, INPUT);
        touch_.setPins(-1, TOUCH_INT);
        touch_present_ = touch_.begin(Wire, TOUCH_ADDR_A) || touch_.begin(Wire, TOUCH_ADDR_B);
        if (touch_present_) {
            touch_.setMaxCoordinates(device.width(), device.height());
            touch_.setSwapXY(true);
            touch_.setMirrorXY(false, true);
        }
        MAP_LOG("TDeckTouch result %d after, supports %d points", touch_present_, touch_.getSupportTouchPoint());
        return touch_present_;
    }
    void end() override { }
    bool iterate(uint32_t now) override {
        if (!touch_present_ || !ctrl_) return false;

        bool pressed = touch_.isPressed();
        const auto& touches = touch_.getTouchPoints();

        if (pressed && touches.getPointCount() == 0 || (pressed == lastPressed_))
            return true; //no update for us

        BaseTouchPoint tp;
        if (touches.hasPoints()) {
            //TODO multi-touch!
            tp.x = touches[0].x;
            tp.y = touches[0].y;
            tp.pressed = true;
        } else { // potential release event
            if ((now - lastPressChange_) < pressDebounceMs_) {
                return true; //skip this one
            }
            tp.pressed = false;
        }
        lastPressed_ = pressed;
        lastPressChange_ = now;
        ctrl_->onTouch(tp, now);
        return true;
    }
    void onSleep(bool sleeping) override {
        if (sleeping) touch_.sleep();
        else touch_.wakeup();
    }
};

static volatile uint32_t s_trackball_up = 0, s_trackball_down = 0;
static volatile uint32_t s_trackball_left = 0, s_trackball_right = 0;

void IRAM_ATTR isr_trackball_up() { s_trackball_up++; }
void IRAM_ATTR isr_trackball_down() { s_trackball_down++; }
void IRAM_ATTR isr_trackball_left() { s_trackball_left++; }
void IRAM_ATTR isr_trackball_right() { s_trackball_right++; }

class TDeckTrackball : public InputBase {
    bool trackball_enabled_ = false;
    bool prevSelState_ = false;
    uint32_t prevSelChange_ = 0;
    int16_t trackball_x_ = 0, trackball_y_ = 0; //accumulates
    const int maxUpdateMs_ = 10; //100hz
    uint32_t lastUpdateScroll_ = 0;
public:
    bool begin(ControllerBase* ctrl) override {
        InputBase::begin(ctrl);
        MAP_LOG("TDeckTrackball", "Starting trackball on GPIO %d,%d,%d,%d (press %d)",
                TRACKBALL_UP, TRACKBALL_DOWN, TRACKBALL_LEFT, TRACKBALL_RIGHT, TRACKBALL_SEL);
        // Set up GPIO for trackball
        pinMode(TRACKBALL_SEL, INPUT_PULLUP);
        pinMode(TRACKBALL_UP, INPUT_PULLUP);
        pinMode(TRACKBALL_DOWN, INPUT_PULLUP);
        pinMode(TRACKBALL_LEFT, INPUT_PULLUP);
        pinMode(TRACKBALL_RIGHT, INPUT_PULLUP);
        onSleep(false);
        return true;
    }
    void end() override { onSleep(true); }

    bool iterate(uint32_t now) override {
        // Poll trackball press button (GPIO0, active-low), always even if ball is disabled
        bool sel = !digitalRead(TRACKBALL_SEL);
        bool selChanged = false;

        // Debounce: only fire on falling edge with 100ms hysteresis
        if (sel != prevSelState_ && ((now - prevSelChange_) >= maxUpdateMs_)) {
            prevSelState_ = sel;
            prevSelChange_ = now;
            selChanged = true;
        }

        if (!trackball_enabled_ && selChanged) { //disabled scrolling? still notify select
            TrackballDelta td;
            td.pressed = sel;
            ctrl_->onScroll(td, now);
            return true;
        }

        // Atomically read ISR counters
        uint32_t up, down, left, right;
        noInterrupts();
        up = s_trackball_up; s_trackball_up = 0;
        down = s_trackball_down; s_trackball_down = 0;
        left = s_trackball_left; s_trackball_left = 0;
        right = s_trackball_right; s_trackball_right = 0;
        interrupts();

        // Accumulate deltas
        trackball_x_ += (int16_t)(right - left);
        trackball_y_ += (int16_t)(up - down);

        // Send delta if we've accumulated movement
        bool sendupdate = selChanged; //select is already debounced! always update when it wants to
        sendupdate |= (trackball_x_ != 0 || trackball_y_ != 0) && ((now - lastUpdateScroll_) > maxUpdateMs_);
        if (sendupdate) {
            lastUpdateScroll_ = now;
            TrackballDelta td;
            td.dx = trackball_x_;
            td.dy = trackball_y_;
            td.pressed = prevSelState_;
            ctrl_->onScroll(td, now);
            trackball_x_ = 0;
            trackball_y_ = 0;
        }
        return false;
    }

    void onSleep(bool sleeping) override {
        if (sleeping) {
            detachInterrupt(TRACKBALL_UP);
            detachInterrupt(TRACKBALL_DOWN);
            detachInterrupt(TRACKBALL_LEFT);
            detachInterrupt(TRACKBALL_RIGHT);
            trackball_enabled_ = false;
        } else { // Attach ISRs (FALLING edge for active-low)
            attachInterrupt(TRACKBALL_UP, isr_trackball_up, FALLING);
            attachInterrupt(TRACKBALL_DOWN, isr_trackball_down, FALLING);
            attachInterrupt(TRACKBALL_LEFT, isr_trackball_left, FALLING);
            attachInterrupt(TRACKBALL_RIGHT, isr_trackball_right, FALLING);
            trackball_enabled_ = true;
        }
    }
};

class TDeckBattery : public BattManagerBase {
public:
    void setup() override { pinMode(PIN_BAT_ADC, INPUT); }
    float getMV() const override {
        const uint8_t samples = 8;
        uint32_t raw = 0;
        for (uint8_t i = 0; i < samples; i++)
            raw += analogRead(PIN_BAT_ADC);
        raw /= samples;
        return ((BAT_ADC_MULT * (float)raw) / 4096.0f);
    }
    float getMVtoPercent(uint16_t mv) const override {
        static const uint16_t table[][2] = {
            {4200, 100}, {4050, 90}, {3950, 80}, {3850, 70}, {3800, 60},
            {3750, 50},  {3700, 40}, {3650, 30}, {3600, 20}, {3550, 10},
            {3400, 5},   {3200, 0}
        };
        const uint8_t entries = sizeof(table) / sizeof(table[0]);
        if (mv >= table[0][0]) return 100;
        if (mv <= table[entries - 1][0]) return 0;
        for (uint8_t i = 0; i < entries - 1; i++) {
            if (mv >= table[i + 1][0]) {
                float fraction = (float)(mv - table[i + 1][0]) / (table[i][0] - table[i + 1][0]);
                return table[i + 1][1] + (float)(fraction * (table[i][1] - table[i + 1][1]));
            }
        }
        return 0;
    }
};

static TDeckTrackball tdeck_trackball;
static TDeckKeyboard tdeck_keyboard;
static TDeckTouch tdeck_touch;
static TDeckBattery tdeck_batt;

// Call this in your setup, after creating ctrl:
void setupTDeckInput(Controller* c) {
    bool i2cinit = false;
    for (uint8_t i = 0; i < 20; i++) {
        if ((i2cinit = Wire.begin(SHARED_I2C_SDA, SHARED_I2C_SCL)))
            break;
        delay(5);
    }
    MAP_LOG("I2C Wire.begin %d", i2cinit);
    for (InputBase* ip : {(InputBase*)&tdeck_trackball, (InputBase*)&tdeck_touch, (InputBase*)&tdeck_keyboard}) {
        uint8_t tries = 0;
        bool res = false;
        while (tries++ < 20 && ! ((res = ip->begin(c))) ) {
            delay(10);
        }
        MAP_LOG("input res(%d) after %d tries", res, tries);
        c->addInput(ip);
    }
    tdeck_batt.setup();
    c->setBattMgr(&tdeck_batt);
}
