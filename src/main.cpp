#include <Arduino.h>
#include <M5Unified.h>
#include <M5Cardputer.h>
#include <SD.h>
#include <lvgl.h>
#include <log.h>
#include "controller.h"
#include "version.h"

// ── LVGL display + touch buffers ─────────────────────────────────────────────
static lv_disp_draw_buf_t _dispBuf;
static lv_color_t* _buf1; //allocated below
static lv_color_t* _buf2;
static lv_disp_drv_t      _dispDrv;

static void _setupLvgl(int dispW, int dispH);

static constexpr int SD_SPI_SCK_PIN  = 40;
static constexpr int SD_SPI_MISO_PIN = 39;
static constexpr int SD_SPI_MOSI_PIN = 14;
static constexpr int SD_SPI_CS_PIN   = 12;

static Controller ctrl(GIT_VERSION);

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5Cardputer.begin();
    M5.Display.setRotation(1);  // landscape

    Serial.begin(115200);
    MAP_LOG("Starting up (free: %u)", ESP.getFreeHeap());

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        // g_state.sdMounted = true;
        // SD.mkdir(GPX_ROOT);
        MAP_LOG("[SD] Mounted. size %dm type %d", SD.cardSize()/1000000, SD.cardType());
    } else {
        MAP_LOG("[SD] Not found — tile/GPX features disabled");
    }

    _setupLvgl(M5.Lcd.width(), M5.Lcd.height());

    lv_obj_t* screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    ctrl.setup(screen);

    lv_task_handler();
    MAP_LOG("Setup complete (free %d) version %s", ESP.getFreeHeap(), GIT_VERSION);
}

void loop() {
    M5.update();
    uint32_t now = millis();
    ctrl.iterate(now);
    lv_task_handler();
    yield();
}

static void _setupLvgl(int dispW, int dispH) {
    MAP_LOG("lvgl init start %dx%d (free: %u)", dispW, dispH, ESP.getFreeHeap());
    lv_init();
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
        M5.Display.startWrite();
        M5.Display.setAddrWindow(area->x1, area->y1, w, h);
        // M5Unified pushPixels accepts RGB565
        M5.Display.pushPixels((uint16_t*)data, w * h, true);
        M5.Display.endWrite();
        lv_disp_flush_ready(drv);
    };
    _dispDrv.draw_buf = &_dispBuf;
    _dispDrv.full_refresh = 0;
    lv_disp_drv_register(&_dispDrv);
    MAP_LOG("lvgl init done (free: %u)", ESP.getFreeHeap());
}
