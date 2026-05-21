#pragma once

#include <stdint.h>
#include <vector>
#include <lvgl.h>
#include <algorithm>
#include "PixelBuffer.h"

class MapRenderer {
public:
    struct TileCacheEntry {
        int z = -1, x = -1, y = -1;
        PixelBuffer buffer;
        uint32_t lastUsed = 0;
        bool is(int ox, int oy, int oz) const { return ox == x && oy == y && oz == z; }
        const PixelBuffer* load(int ox, int oy, int oz, const char* fmt, uint32_t age);
    };
    static constexpr uint8_t TILECACHE_SIZE = 4;

    MapRenderer() = default;
    ~MapRenderer();

    bool begin(lv_obj_t* parent, lv_color_t* canvas_buffer, uint16_t w, uint16_t h, const char* fmt, uint16_t tileSize = 256);

    void setTilePathPattern(const char* pattern);
    void handleDraw(lv_event_t* e = nullptr);
    void project(double lat, double lon, lv_coord_t& px, lv_coord_t& py) const; /// get display px position of a lat/lon point
    bool isVisible(lv_coord_t px, lv_coord_t py) const;
    void drawPosDot(double lat, double lon);
    void drawHomeMarker(double lat, double lon);

    lv_obj_t* canvas() { return canvas_; }
    int   zoom() const { return zoom_; }
    double lat() const { return lat_; }
    double lon() const { return lon_; }

    void setCenter(double lat, double lon) { lat_ = lat; lon_ = lon; }
    void setZoom(int z) { zoom_ = (z < 0) ? 0 : (z > 20 ? 20 : z); }
    void panPx(int dx, int dy);

private:
    lv_obj_t* canvas_ = nullptr;
    lv_color_t* canvas_buf_ = nullptr;  // Points to canvas's existing buffer
    int16_t x_ = 0, y_ = 0;
    uint16_t width_ = 0, height_ = 0;
    uint16_t       tileSize_;
    const char*    pathPattern_ = nullptr;
    double         lat_ = 0, lon_ = 0;
    int            zoom_ = 10;
    TileCacheEntry cache_[TILECACHE_SIZE];
    uint32_t       renderCount_ = 0;

public: //settings
    uint32_t colBg_ = 0x000000;
    uint32_t colAccent_ = 0xFF0000;
    uint32_t colHome_ = 0x00FF00;

public: //utility methods, helpful to unit test
    static void _latLonToTileF(double lat, double lon, int z, double& tx, double& ty);
    static double _latToTileY(double lat, int z);
    static double _tileYToLat(double ty, int z);
    const PixelBuffer* _getTile(int z, int x, int y);
    void _drawTiles();
    void _drawNoTile(int px, int py);
};
