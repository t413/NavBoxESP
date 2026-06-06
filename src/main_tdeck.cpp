#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <navboxlib/log.h>
#include <basetilewriter.h>
#include "controller.h"
#include "version.h"

// Default pin definitions (can be overridden by build_flags)
constexpr int PIN_POWER_ON  = 10;
constexpr int SHARED_SCK    = 40;
constexpr int SHARED_MISO   = 38;
constexpr int SHARED_MOSI   = 41;
constexpr int SDCARD_CS     = 39;
constexpr int TFT_CS        = 12;
constexpr int TFT_BL        = 42;
constexpr int TFT_DC        = 11;
constexpr int TFT_WIDTH     = 240; //rotated later
constexpr int TFT_HEIGHT    = 320;
constexpr int ROTATION      = 1;
constexpr int KB_I2C_SDA    = 18;
constexpr int KB_I2C_SCL    = 8;
constexpr int KB_I2S_ADDR   = 0x55;
constexpr int TRACKBALL_SEL     = 0;
constexpr int TRACKBALL_UP      = 3;
constexpr int TRACKBALL_DOWN    = 15;
constexpr int TRACKBALL_LEFT    = 2;
constexpr int TRACKBALL_RIGHT   = 1;
constexpr int TOUCH_SDA     = 18;  // Same I2C as keyboard
constexpr int TOUCH_SCL     = 8;
constexpr int TOUCH_INT     = 16;
constexpr int TOUCH_ADDR    = 0x14;  // GT911_SLAVE_ADDRESS_L
constexpr int GPS_RX = 44;
constexpr int GPS_TX = 43;
constexpr auto SPIBUS_HOST = FSPI;

SPIClass spibus(SPIBUS_HOST);

// LVGL display + touch buffers
static lv_disp_draw_buf_t _dispBuf;
static lv_color_t* _buf1; //allocated below
static lv_color_t* _buf2;
static lv_disp_drv_t _dispDrv;

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

LGFX device;

static void _setupLvgl(int dispW, int dispH);

void setup() {
    Serial.begin(115200);
    MAP_LOG("T-Deck startup (free: %u)", ESP.getFreeHeap());

    // power on pin
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);

    // init I2C (keyboard/brightness helpers on some T-Deck variants)
    Wire.begin();

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    auto iret = device.init();
    // device.setRotation(ROTATION);
    device.fillScreen(0);
    device.setTextColor(0x58A6FF); // COL_ACCENT
    device.setTextDatum(middle_center);
    device.drawString("NavBox Loading...", device.width() / 2, device.height() / 2, &fonts::FreeSansBold12pt7b);

    _setupLvgl(device.width(), device.height());
    MAP_LOG("Display Init %d: %dx%d, colorDepth=%d, r %d", iret, device.width(), device.height(),  device.getColorDepth(), device.getRotation());

    lv_obj_t* screen = lv_scr_act();
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
    ctrl.setupGPS(GPS_RX, GPS_TX, 38400, Serial2);

    lv_task_handler();
    MAP_LOG("T-Deck setup complete (free %d) version %s", ESP.getFreeHeap(), GIT_VERSION);
}

void loop() {
    uint32_t now = lgfx::millis();
    ctrl.iterate(now);
    lv_task_handler();
    yield();
}

static void _setupLvgl(int dispW, int dispH) {
    MAP_LOG("lvgl init start %dx%d (free: %u)", dispW, dispH, ESP.getFreeHeap());
    lv_init();

    // const int bufsize = dispW * dispH;  // Full screen
    // _buf2 = nullptr;  // Single buffer mode
    // _dispDrv.full_refresh = 1;  // Full refresh mode for full buffer
    const int bufsize = dispW * 20;
    _buf1 = (lv_color_t*)malloc(sizeof(lv_color_t) * bufsize);
    _buf2 = (lv_color_t*)malloc(sizeof(lv_color_t) * bufsize);
    MAP_LOG("lvgl init made bufs (free: %u)", ESP.getFreeHeap());

    lv_disp_draw_buf_init(&_dispBuf, _buf1, _buf2, bufsize);
    lv_disp_drv_init(&_dispDrv);
    _dispDrv.hor_res  = dispW;
    _dispDrv.ver_res  = dispH;
    _dispDrv.flush_cb = [](lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* data) {
        uint32_t w = area->x2 - area->x1 + 1;
        uint32_t h = area->y2 - area->y1 + 1;
        device.startWrite();
        device.setAddrWindow(area->x1, area->y1, w, h);
        device.writePixels((lgfx::rgb565_t*)data, w * h);
        device.endWrite();
        lv_disp_flush_ready(drv);
    };
    _dispDrv.draw_buf = &_dispBuf;
    _dispDrv.full_refresh = 0;
    lv_disp_drv_register(&_dispDrv);
    MAP_LOG("lvgl init done (free: %u)", ESP.getFreeHeap());
}
