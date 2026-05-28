#include "MapRenderer.h"
#include "log.h"
#include "TrackLog.h"
#include "MapLayer.h"
#include "fileclass.h"
#include <lvgl.h>

const PixelBuffer* MapRenderer::TileCacheEntry::load(int ox, int oy, int oz, const char* fmt, const Bounds& bounds) {
    char path[128];
    snprintf(path, sizeof(path), fmt, oz, ox, oy);
    if (buffer.loadImg(path, bounds)) {
        z = oz, x = ox, y = oy, onscreen = 0;
        dsc_.header.always_zero = 0;
        dsc_.header.w = buffer.width_;
        dsc_.header.h = buffer.height_;
        dsc_.data_size = buffer.width_ * buffer.height_ * sizeof(uint16_t);
        dsc_.header.cf = LV_IMG_CF_TRUE_COLOR;
        dsc_.data = (const uint8_t*)buffer.data_;
        return &buffer;
    } else MAP_LOG("tile::load failed loading %s (free: %u)", path, freeHeap());
    return nullptr;
}

void MapRenderer::TileCacheEntry::update(int px, int py, bool visible, zoom_t magnification) {
    if (visible) {
        lv_coord_t adjustedX = px + (lv_coord_t)(buffer.getOffsetX() * magnification);
        lv_coord_t adjustedY = py + (lv_coord_t)(buffer.getOffsetY() * magnification);
        lv_img_set_pivot(img_obj, 0, 0);
        lv_img_set_zoom(img_obj, (uint16_t)(magnification * buffer.uncroppedW_)); //TODO allow fractional zoom
        lv_obj_set_pos(img_obj, adjustedX, adjustedY);
        lv_obj_clear_flag(img_obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(img_obj, LV_OBJ_FLAG_HIDDEN);
    }
    onscreen = visible;
}

void MapRenderer::TileCacheEntry::clear() {
    z = -1, x = -1, y = -1;
    update(0,0,false);
    buffer.clear();
}

MapRenderer::~MapRenderer() {
    for (auto& t : cache_)
        t.buffer.clear();
    for (auto& layer : layers_) {
        if (layer == markerLayer_) markerLayer_ = nullptr;
        delete layer;
        layer = nullptr;
    }
    if (obj_) lv_obj_del(obj_);  // Parent container
    obj_ = nullptr;
}

bool MapRenderer::begin(lv_obj_t* parent, uint16_t w, uint16_t h, const char* fmt, uint16_t tileSize) {
    width_ = w;
    height_ = h;
    tileSize_ = tileSize;
    pathPattern_ = fmt;

#ifdef ARDUINO
    cropmode_ = !psramFound();
#endif

    if (!parent) {
        return false;
    }
    obj_ = lv_obj_create(parent);
    lv_obj_set_size(obj_, width_, height_);
    lv_obj_set_pos(obj_, x_, y_);
    lv_obj_set_style_pad_all(obj_, 0, 0);
    lv_obj_set_style_border_width(obj_, 0, 0);
    lv_obj_set_style_radius(obj_, 0, 0);
    lv_obj_set_scrollbar_mode(obj_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(obj_, lv_color_hex(colBg_), 0);

    // Create fixed image objects for the 2x2 grid
    for (uint8_t i = 0; i < TILECACHE_SIZE; i++) {
        cache_[i].img_obj = lv_img_create(obj_);
        lv_img_set_pivot(cache_[i].img_obj, 0, 0);
        lv_obj_add_flag(cache_[i].img_obj, LV_OBJ_FLAG_HIDDEN);
    }
    return true;
}

void MapRenderer::setXY(uint16_t x, uint16_t y) { lv_obj_set_pos(obj_, (x_ = x), (y_ = y)); }

void MapRenderer::invalidate() {
    if (cropmode_) { //invalidate cache first
        MAP_LOG("cropmode enabled, clearing tile cache on recenter");
        for (auto& t : cache_)
            t.clear();
    }
    _updateTiles();
}

bool MapRenderer::project(double lat, double lon, lv_coord_t& px, lv_coord_t& py) const {
    const uint16_t sts = scaledTileSize();
    double tx, ty, cx, cy;
    _latLonToTileF(lat, lon, zoom_, tx, ty);
    _latLonToTileF(mapCenter_.lat(), mapCenter_.lon(), zoom_, cx, cy);
    px = (lv_coord_t)round((tx - cx) * sts + width_  / 2);
    py = (lv_coord_t)round((ty - cy) * sts + height_ / 2);
    return isVisible(px, py);
}

bool MapRenderer::isVisible(lv_coord_t px, lv_coord_t py) const {
    return px >= 0 && px < width_ && py >= 0 && py < height_;
}

void MapRenderer::setCenter(const GeoPoint& p, zoom_t zoom) {
    mapCenter_ = p;
    if (zoom > 0 && zoom < 20) zoom_ = zoom;
    invalidate();
}

void MapRenderer::setZoom(zoom_t zoom, zoom_t magnification) {
    if (magnification == MAGNF_AUTO) {
        zoom_ = findBestZoomWithTiles(mapCenter_, zoom);
        magnification = zoom - zoom_ + 1;
        zoom = zoom_;
    }
    if (zoom != ZOOM_UNCHANGED)
        zoom_          = (zoom > 20) ? 20 : zoom;
    magnification_ = (magnification < 1) ? 1 : magnification;
    MAP_LOG("setZoom z%d mag%d (scaledTile=%d)", zoom_, magnification_, scaledTileSize());
    invalidate();
}

zoom_t MapRenderer::findBestZoomWithTiles(const GeoPoint &cntr, zoom_t start) {
    //zoom out until finding tiles for this zoom level
    char path[128];
    for (zoom_t z = start; z > 0; z--) {
        double tx, ty;
        _latLonToTileF(cntr.lat(), cntr.lon(), z, tx, ty);
        snprintf(path, sizeof(path), pathPattern_, z, (int)tx, (int)ty);
        bool has = SD.exists(path);
        MAP_LOG("find zoom %s -> %d", path, has);
        if (has) { return z; } //found
    }
    return start;
}

void MapRenderer::panPx(int16_t dx, int16_t dy) {
    const uint16_t sts   = scaledTileSize(); //already accounts for magnification
    const double totalPx = (double)sts * pow(2.0, zoom_); // total world width in screen-pixels
    double ty = _latToTileY(mapCenter_.lat(), zoom_) + (double)dy / sts;
    setCenter({_tileYToLat(ty, zoom_), mapCenter_.lon() + dx / totalPx * 360.0});
}


void MapRenderer::addLayer(MapLayer* layer) {
    layers_.push_back(layer);
}

void MapRenderer::removeLayer(MapLayer* layer) {
    layers_.erase(std::remove_if(layers_.begin(), layers_.end(),
        [layer](MapLayer* ptr) { return ptr == layer; }), layers_.end());
    if (markerLayer_ == layer) markerLayer_ = nullptr;
}

MarkerLayer* MapRenderer::getMarkerLayer() {
    if (!markerLayer_)
        addLayer( (markerLayer_ = new MarkerLayer(this)) );
    return markerLayer_;
}

MapRenderer::XY MapRenderer::getMarkerPx(uint16_t id) const {
    if (!markerLayer_) return XY{-1,-1};
    const auto& mo = markerLayer_->findObj(id);
    if (!mo.obj_ || lv_obj_has_flag(mo.obj_, LV_OBJ_FLAG_HIDDEN))
        return XY{-1, -1};
    return XY{
        (int16_t)(lv_obj_get_x(mo.obj_) + mo.m_.size / 2),
        (int16_t)(lv_obj_get_y(mo.obj_) + mo.m_.size / 2)
    };
}

int16_t MapRenderer::getPxDistToCenter(const MapRenderer::XY &pos) const {
    int16_t dx = pos.x - (int16_t)(width_ / 2);
    int16_t dy = pos.y - (int16_t)(height_ / 2);
    return (int16_t)sqrt(dx * dx + dy * dy);
}


// Private update methods

void MapRenderer::_updateLayers() {
    for (auto& layer : layers_) {
        layer->update();
    }
}

void MapRenderer::_updateTiles() {
    if (!obj_) return;

    const uint16_t sts = scaledTileSize(); // effective on-screen pixels per tile
    double tx, ty;
    _latLonToTileF(mapCenter_.lat(), mapCenter_.lon(), zoom_, tx, ty);
    int nTiles = (int)pow(2.0, zoom_);

    // Top-left tile index and its pixel origin on screen
    const int tx_s = (int)floor(tx - (double)width_  / 2 / sts);
    const int ty_s = (int)floor(ty - (double)height_ / 2 / sts);
    const int px_s = (int)round((tx_s - tx) * sts + width_  / 2.0);
    const int py_s = (int)round((ty_s - ty) * sts + height_ / 2.0);

    for (int j = 0; j < TILECACHE_SIZE; j++) cache_[j].onscreen = 0; //reset all
    struct TileSlot { int x,y; int tx,ty; };
    TileSlot missing[TILECACHE_SIZE] = {0};
    uint8_t missingIdx = 0;

    // Step 1: Identify and mark existing tiles in cache
    for (uint8_t i = 0; i < 4; i++) {
        const int x = ((tx_s + (i % 2)) % nTiles + nTiles) % nTiles; // osm [z/x/y]
        const int y = ty_s + (i / 2);
        if (y < 0 || y >= nTiles) continue;
        const int tpx = px_s + (i % 2) * (int)sts; // on-screen pixel dest
        const int tpy = py_s + (i / 2) * (int)sts;

        int idx = _findTile(x, y, zoom_);
        if (idx != -1) { //found pre-loaded
            cache_[idx].update(tpx, tpy, true, magnification_);
        } else { //add to missing
            missing[missingIdx++] = { x, y, tpx, tpy };
        }
    }

    // Step 2: Load missing tiles into oldest available slots
    for (uint8_t slotidx = 0; slotidx < missingIdx; slotidx++) {
        const auto& m = missing[slotidx];

        int newIdx = _findSlot();
        if (newIdx != -1) { // found empty slot
            auto& tile = cache_[newIdx];
            Bounds crop;
            if (cropmode_) {
                crop.left  = (coord_t)std::max(0, (int)(-m.tx / magnification_));
                crop.top   = (coord_t)std::max(0, (int)(-m.ty / magnification_));
                crop.right = (coord_t)std::min((int)tileSize_, (int)((width_  - m.tx) / magnification_));
                crop.bttm  = (coord_t)std::min((int)tileSize_, (int)((height_ - m.ty) / magnification_));
            }

            if (tile.load(m.x, m.y, zoom_, pathPattern_, crop)) {
                lv_img_set_src(tile.img_obj, &tile.dsc_);
                tile.update(m.tx, m.ty, true, magnification_);
            } else {
                tile.z = -1;
            }
        }
    }

    for (int j = 0; j < TILECACHE_SIZE; j++)
        if (!cache_[j].onscreen)
            cache_[j].update(0, 0, false);

    _updateLayers();
}

// Static geo math

void MapRenderer::_latLonToTileF(double lat, double lon, int z, double& tx, double& ty) {
    double n = pow(2.0, z);
    tx = n * (lon + 180.0) / 360.0;
    double lrad = lat * DEG_2_RAD;
    ty = n * (1.0 - log(tan(lrad) + 1.0/cos(lrad)) / M_PI) / 2.0;
}

double MapRenderer::_latToTileY(double lat, int z) {
    double n = pow(2.0, z);
    double lrad = lat * DEG_2_RAD;
    return n * (1.0 - log(tan(lrad) + 1.0/cos(lrad)) / M_PI) / 2.0;
}

double MapRenderer::_tileYToLat(double ty, int z) {
    double n = pow(2.0, z);
    double sinh_val = sinh(M_PI * (1.0 - 2.0 * ty / n));
    return atan(sinh_val) / DEG_2_RAD;
}

// Cache helpers

int MapRenderer::_findTile(int x, int y, int z) {
    for (int i = 0; i < TILECACHE_SIZE; i++) {
        if (cache_[i].is(x, y, z)) return i;
    }
    return -1;
}

int MapRenderer::_findSlot() {
    for (int j = 0; j < TILECACHE_SIZE; j++)
        if (!cache_[j].onscreen)
            return j;
    return -1;
}
int freeHeap() {
    #ifdef ARDUINO
    return ESP.getFreeHeap();
    #else
    return -1;
    #endif
}
