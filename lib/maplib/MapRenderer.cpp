#include "MapRenderer.h"
#include "log.h"

static constexpr const double DEG_2_RAD = M_PI / 180;

const PixelBuffer* MapRenderer::TileCacheEntry::load(int ox, int oy, int oz, const char* fmt, const Bounds& bounds) {
    char path[128];
    snprintf(path, sizeof(path), fmt, oz, ox, oy);
    MAP_LOG("tile::load trying %s (free: %u)", path, freeHeap());
    if (buffer.loadImg(path, bounds)) {
        z = oz, x = ox, y = oy, onscreen = 0;
        dsc_.header.always_zero = 0;
        dsc_.header.w = buffer.width_;
        dsc_.header.h = buffer.height_;
        dsc_.data_size = buffer.width_ * buffer.height_ * sizeof(uint16_t);
        dsc_.header.cf = LV_IMG_CF_TRUE_COLOR;
        dsc_.data = (const uint8_t*)buffer.data_;
        MAP_LOG("tile::load found %s", path);
        return &buffer;
    }
    MAP_LOG("tile::load missing %s", path);
    return nullptr;
}

MapRenderer::~MapRenderer() { }

bool MapRenderer::begin(lv_obj_t* parent, uint16_t w, uint16_t h, const char* fmt, uint16_t tileSize) {
    width_ = w;
    height_ = h;
    tileSize_ = tileSize;
    pathPattern_ = fmt;

    if (!parent) {
        return false;
    }
    obj_ = lv_obj_create(parent);
    lv_obj_set_size(obj_, width_, height_);
    lv_obj_set_style_bg_color(obj_, lv_color_hex(colBg_), 0);

    // Create fixed image objects for the 2x2 grid
    for (uint8_t i = 0; i < TILECACHE_SIZE; i++) {
        cache_[i].img_obj = lv_img_create(obj_);
        lv_obj_add_flag(cache_[i].img_obj, LV_OBJ_FLAG_HIDDEN);
    }

    // Initialize standard LVGL objects for markers
    homeMarker_ = lv_obj_create(obj_);
    lv_obj_set_size(homeMarker_, 16, 15);
    lv_obj_set_style_radius(homeMarker_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(homeMarker_, lv_color_hex(colHome_), 0);
    lv_obj_set_style_border_color(homeMarker_, lv_color_white(), 0);
    lv_obj_set_style_border_width(homeMarker_, 2, 0);
    lv_obj_t* label = lv_label_create(homeMarker_);
    lv_label_set_text(label, "H");
    lv_obj_center(label);

    posDot_ = lv_obj_create(obj_);
    lv_obj_set_size(posDot_, 13, 13);
    lv_obj_set_style_radius(posDot_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(posDot_, lv_color_hex(colAccent_), 0);
    lv_obj_set_style_border_color(posDot_, lv_color_white(), 0);
    lv_obj_set_style_border_width(posDot_, 2, 0);
    return true;
}

void MapRenderer::invalidate() {
    _updateTiles();
}

void MapRenderer::project(double lat, double lon, lv_coord_t& px, lv_coord_t& py) const {
    double tx, ty;
    _latLonToTileF(lat, lon, zoom_, tx, ty);
    double cx, cy;
    _latLonToTileF(lat_, lon_, zoom_, cx, cy);
    px = (lv_coord_t)round((tx - cx) * tileSize_ + width_ / 2);
    py = (lv_coord_t)round((ty - cy) * tileSize_ + height_ / 2);
}

void MapRenderer::drawPosDot(double lat, double lon) {
    lv_coord_t px, py;
    project(lat, lon, px, py);
    if (posDot_) {
        if (isVisible(px, py)) {
            lv_obj_clear_flag(posDot_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(posDot_, px - 5, py - 5);
        } else lv_obj_add_flag(posDot_, LV_OBJ_FLAG_HIDDEN);
    }
}

bool MapRenderer::isVisible(lv_coord_t px, lv_coord_t py) const {
    return px >= x_ && px < (width_ + x_) && py >= y_ && py < (height_ + y_);
}

void MapRenderer::drawHomeMarker(double lat, double lon) {
    lv_coord_t px, py;
    project(lat, lon, px, py);

    if (homeMarker_) {
        if (isVisible(px, py)) {
            lv_obj_clear_flag(homeMarker_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(homeMarker_, px - 8, py - 8);
        } else lv_obj_add_flag(homeMarker_, LV_OBJ_FLAG_HIDDEN);
    }
}

void MapRenderer::panPx(int dx, int dy) {
    double scale = (double)tileSize_ * pow(2.0, zoom_);
    lon_ += dx / scale * 360.0;
    double ty = _latToTileY(lat_, zoom_) + (double)dy / tileSize_;
    lat_ = _tileYToLat(ty, zoom_);
    _updateTiles();
}

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

void MapRenderer::_updateTiles() {
    if (!obj_) return;
    double tx, ty; _latLonToTileF(lat_, lon_, zoom_, tx, ty);
    int nTiles = (int)pow(2.0, zoom_);
    int tx_s = (int)floor(tx - (double)width_ / 2 / tileSize_);
    int ty_s = (int)floor(ty - (double)height_ / 2 / tileSize_);
    lv_coord_t px_s = (lv_coord_t)round((tx_s - tx) * tileSize_ + width_ / 2.0);
    lv_coord_t py_s = (lv_coord_t)round((ty_s - ty) * tileSize_ + height_ / 2.0);

    bool busy[TILECACHE_SIZE] = {false};
    struct Slot { int x, y; lv_coord_t px, py; bool ok; } slots[4];

    for (int i = 0; i < 4; i++) {
        slots[i] = { ((tx_s + (i % 2)) % nTiles + nTiles) % nTiles, ty_s + (i / 2),
                     (lv_coord_t)(px_s + (i % 2) * tileSize_), (lv_coord_t)(py_s + (i / 2) * tileSize_), false };
        if (slots[i].y < 0 || slots[i].y >= nTiles) continue;

        // Step 1: Check cache for existing tile
        for (int j = 0; j < TILECACHE_SIZE; j++) {
            if (cache_[j].is(slots[i].x, slots[i].y, zoom_)) {
                _updateTileObj(j, slots[i].px, slots[i].py, true);
                busy[j] = slots[i].ok = true;
                break;
            }
        }
    }

    // Step 2: Load missing tiles into oldest unused slots
    for (int i = 3; i >= 0; i--) {
        if (slots[i].ok || slots[i].y < 0 || slots[i].y >= nTiles) continue;
        int best = -1; uint32_t oldest = 0xFFFFFFFF;
        for (int j = 0; j < TILECACHE_SIZE; j++) {
            if (!busy[j] && cache_[j].lastUsed < oldest) { oldest = cache_[j].lastUsed; best = j; }
        }
        if (best != -1) {
            busy[best] = true;
            if (cache_[best].load(slots[i].x, slots[i].y, zoom_, pathPattern_, ++renderCount_)) {
                lv_img_set_src(cache_[best].img_obj, &cache_[best].dsc_);
                _updateTileObj(best, slots[i].px, slots[i].py, true);
            } else {
                _updateTileObj(best, 0, 0, false);
                cache_[best].z = -1;
            }
        }
    }

    for (int j = 0; j < TILECACHE_SIZE; j++) if (!busy[j]) _updateTileObj(j, 0, 0, false);
}

void MapRenderer::_updateTileObj(int idx, lv_coord_t x, lv_coord_t y, bool visible) {
    if (visible) {
        lv_coord_t adjustedX = x + cache_[idx].buffer.getOffsetX();
        lv_coord_t adjustedY = y + cache_[idx].buffer.getOffsetY();
        lv_obj_set_pos(cache_[idx].img_obj, adjustedX, adjustedY);
        lv_obj_clear_flag(cache_[idx].img_obj, LV_OBJ_FLAG_HIDDEN);
        cache_[idx].lastUsed = renderCount_;
    } else {
        lv_obj_add_flag(cache_[idx].img_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

int freeHeap() {
    #ifdef ARDUINO
    return ESP.getFreeHeap();
    #else
    return -1;
    #endif
}
