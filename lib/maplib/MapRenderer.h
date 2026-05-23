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
        lv_img_dsc_t dsc_{};
        lv_obj_t* img_obj = nullptr;
        int onscreen = 0; //set while iterating over tiles
        bool is(int ox, int oy, int oz) const { return ox == x && oy == y && oz == z; }
        const PixelBuffer* load(int ox, int oy, int oz, const char* fmt, const Bounds &);
    };
    static constexpr uint8_t TILECACHE_SIZE = 4;

    MapRenderer() = default;
    ~MapRenderer();

    bool begin(lv_obj_t* parent, uint16_t w, uint16_t h, const char* fmt, uint16_t tileSize = 256);
    void invalidate();

    void project(double lat, double lon, lv_coord_t& px, lv_coord_t& py) const; /// get display px position of a lat/lon point
    bool isVisible(lv_coord_t px, lv_coord_t py) const;
    void drawPosDot(double lat, double lon);
    void drawHomeMarker(double lat, double lon);

    int   zoom() const { return zoom_; }
    double lat() const { return lat_; }
    double lon() const { return lon_; }

    void setCenter(double lat, double lon) { lat_ = lat; lon_ = lon; }
    void setZoom(int z) { zoom_ = (z < 0) ? 0 : (z > 20 ? 20 : z); }
    void panPx(int dx, int dy);

private:
    lv_obj_t* obj_ = nullptr;
    int16_t x_ = 0, y_ = 0;
    uint16_t width_ = 0, height_ = 0;
    uint16_t       tileSize_;
    const char*    pathPattern_ = nullptr;
    double         lat_ = 0, lon_ = 0;
    int            zoom_ = 14;
    TileCacheEntry cache_[TILECACHE_SIZE];
    uint32_t       renderCount_ = 0;
    lv_obj_t*      posDot_ = nullptr;
    lv_obj_t*      homeMarker_ = nullptr;
    void _updateTileObj(int idx, lv_coord_t x, lv_coord_t y, bool visible);

public: //settings
    uint32_t colBg_ = 0x1A1A1A;
    uint32_t colAccent_ = 0x24b9d7;
    uint32_t colHome_ = 0x2ECC71; // #2ECC71
    bool cropmode_ = false;

public: //utility methods, helpful to unit test
    static void _latLonToTileF(double lat, double lon, int z, double& tx, double& ty);
    static double _latToTileY(double lat, int z);
    static double _tileYToLat(double ty, int z);
    int _findTile(int x, int y, int z);
    int _findSlot();
    void _updateTiles();
};

int freeHeap();
