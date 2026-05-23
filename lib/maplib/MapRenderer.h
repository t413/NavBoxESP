#pragma once

#include <stdint.h>
#include <vector>
#include <lvgl.h>
#include <algorithm>
#include <limits.h>
#include "PixelBuffer.h"

constexpr double DEFAULT_LAT = 37.8044;
constexpr double DEFAULT_LON = -122.2712;
constexpr double NO_LOCATION = (double)(INT_MIN);
constexpr int ZOOM_UNCHANGED = -1;

struct LatLon {
    double lat = 0, lon = 0;
    LatLon(double l = NO_LOCATION, double n = NO_LOCATION) : lat(l), lon(n) { }
    operator bool() const { return lat != NO_LOCATION && lon != NO_LOCATION; }
};

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
    void setXY(uint16_t x, uint16_t y);
    void invalidate();

    bool project(double lat, double lon, lv_coord_t& px, lv_coord_t& py) const; /// get display px position of a lat/lon point
    bool isVisible(lv_coord_t px, lv_coord_t py) const;

    void setCenter(double lat, double lon, int zoom = ZOOM_UNCHANGED); ///Sets the map's center view
    void setZoom(int z);
    void panPx(int dx, int dy);
    void setDot(double lat, double lon);
    void setHome(double lat, double lon);

    int   zoom() const { return zoom_; }
    double lat() const { return mapCenter_.lat; }
    double lon() const { return mapCenter_.lon; }
    LatLon getCenter() const { return mapCenter_; }

private:
    lv_obj_t* obj_ = nullptr;
    int16_t x_ = 0, y_ = 0;
    uint16_t width_ = 0, height_ = 0;
    uint16_t       tileSize_;
    const char*    pathPattern_ = nullptr;
    int            zoom_ = 12;
    TileCacheEntry cache_[TILECACHE_SIZE];
    uint32_t       renderCount_ = 0;
    lv_obj_t*      posDot_ = nullptr;
    lv_obj_t*      homeMarker_ = nullptr;
    LatLon mapCenter_ = {DEFAULT_LAT, DEFAULT_LON};
    LatLon dot_, home_;
    void _updateTileObj(TileCacheEntry& tile, int x, int y, bool visible);

public: //settings
    uint32_t colBg_ = 0x640d5c; // #640d5c
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
    void _updateMarkers();
};

int freeHeap();
