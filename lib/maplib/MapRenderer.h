#pragma once

#include <stdint.h>
#include <vector>
#include <src/draw/lv_img_buf.h>
#include <algorithm>
#include "GeoPoint.h"
#include "PixelBuffer.h"

struct _lv_obj_t;
class MapRenderer;
constexpr double DEFAULT_LAT = 37.8044;
constexpr double DEFAULT_LON = -122.2712;
typedef int8_t zoom_t;
constexpr zoom_t ZOOM_UNCHANGED = -1;
constexpr int8_t MAGNF_AUTO = -1;

class MapLayer;
struct Marker;
class MarkerLayer;


class MapRenderer {
public:
    struct TileCacheEntry {
        int z = -1, x = -1, y = -1;
        PixelBuffer buffer;
        lv_img_dsc_t dsc_{};
        _lv_obj_t* img_obj = nullptr;
        int onscreen = 0; //set while iterating over tiles
        bool is(int ox, int oy, int oz) const { return ox == x && oy == y && oz == z; }
        const PixelBuffer* load(int ox, int oy, int oz, const char* fmt, const Bounds &);
        void update(int px, int py, bool visible, int8_t magnification = 1);
        void clear();
    };
    static constexpr uint8_t TILECACHE_SIZE = 4;

    MapRenderer() = default;
    ~MapRenderer();

    bool begin(_lv_obj_t* parent, uint16_t w, uint16_t h, const char* fmt, uint16_t tileSize = 256);
    void setXY(uint16_t x, uint16_t y);
    void invalidate();

    bool project(double lat, double lon, lv_coord_t& px, lv_coord_t& py) const; /// get display px position of a lat/lon point
    bool isVisible(lv_coord_t px, lv_coord_t py) const;
    zoom_t findBestZoomWithTiles(const GeoPoint &, zoom_t);

    void setCenter(const GeoPoint &, zoom_t zoom = ZOOM_UNCHANGED); ///Sets the map's center view
    void setZoom(zoom_t zoom, zoom_t magnification = MAGNF_AUTO);
    void panPx(int dx, int dy);

    zoom_t zoom() const { return zoom_; }
    zoom_t zoomtotal() const { return zoom_ + magnification_ - 1; }
    zoom_t magnification() const { return magnification_; }
    uint16_t scaledTileSize() const { return tileSize_ * magnification_; }
    double lat() const { return mapCenter_.lat(); }
    double lon() const { return mapCenter_.lon(); }
    const GeoPoint& getCenter() const { return mapCenter_; }
    uint16_t getHeight() const { return height_; }
    uint16_t getWidth()  const { return width_; }

    void addLayer(MapLayer* layer);
    void removeLayer(MapLayer* layer);
    void setLayerVisible(MapLayer* layer, bool visible);
    Marker* addMarker(const Marker& m);

protected:
    _lv_obj_t* obj_ = nullptr;
    int16_t x_ = 0, y_ = 0;
    uint16_t width_ = 0, height_ = 0;
    uint16_t       tileSize_;
    const char*    pathPattern_ = nullptr;
    zoom_t         zoom_ = 12;
    zoom_t         magnification_ = 1;
    TileCacheEntry cache_[TILECACHE_SIZE];
    std::vector<MapLayer*> layers_;
    MarkerLayer* markerLayer_ = nullptr;
    GeoPoint mapCenter_ = {DEFAULT_LAT, DEFAULT_LON};

public: //settings
    uint32_t colBg_ = 0x640d5c; // #640d5c
    uint32_t colAccent_ = 0x24b9d7;
    uint32_t colHome_ = 0x2ECC71; // #2ECC71
    bool cropmode_ = false;
    uint8_t dotsize_ = 14, homesize_ = 16;

public: //utility methods, helpful to unit test
    static void _latLonToTileF(double lat, double lon, int z, double& tx, double& ty);
    static double _latToTileY(double lat, int z);
    static double _tileYToLat(double ty, int z);
    int _findTile(int x, int y, int z);
    int _findSlot();
    void _updateTiles();
    void _updateLayers();
};

int freeHeap();
