#include "MapRenderer.h"
#include "log.h"

static constexpr const double DEG_2_RAD = M_PI / 180;

const PixelBuffer* MapRenderer::TileCacheEntry::load(int ox, int oy, int oz, const char* fmt, uint32_t age) {
    char path[128];
    snprintf(path, sizeof(path), fmt, oz, ox, oy);
    MAP_LOG("tile::load trying %s", path);
    if (buffer.loadImg(path)) {
        z = oz, x = ox, y = oy, lastUsed = age;
        MAP_LOG("tile::load found %s", path);
        return &buffer;
    }
    MAP_LOG("tile::load missing %s", path);
    return nullptr;
}

MapRenderer::~MapRenderer() { }

static void drawEventCb(lv_event_t* e) {
    auto* self = static_cast<MapRenderer*>(lv_event_get_user_data(e));
    self->handleDraw(e);
}

bool MapRenderer::begin(lv_obj_t* parent, lv_color_t* canvas_buffer, uint16_t w, uint16_t h, const char* fmt, uint16_t tileSize) {
    width_ = w;
    height_ = h;
    tileSize_ = tileSize;
    pathPattern_ = fmt;

    if (parent && canvas_buffer) {
        canvas_buf_ = canvas_buffer;  // Use the existing buffer!
        canvas_ = lv_canvas_create(parent);
        lv_canvas_set_buffer(canvas_, canvas_buf_, width_, height_, LV_IMG_CF_TRUE_COLOR);
        lv_obj_set_size(canvas_, width_, height_);
        lv_obj_set_style_bg_color(canvas_, lv_color_hex(colBg_), 0);
        lv_obj_add_event_cb(canvas_, drawEventCb, LV_EVENT_DRAW_MAIN, this);
        return true;
    } else return false;
}

void MapRenderer::handleDraw(lv_event_t* e) {
    lv_canvas_fill_bg(canvas_, lv_color_hex(colBg_), LV_OPA_COVER);
    _drawTiles();
    MAP_LOG("render %5.5f,%5.5f %dz", lat_, lon_, zoom_);
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
    if (!isVisible(px, py)) return;

    lv_draw_rect_dsc_t r;
    lv_draw_rect_dsc_init(&r);
    r.radius = LV_RADIUS_CIRCLE;
    r.bg_color = lv_color_hex(colAccent_);
    r.bg_opa = LV_OPA_COVER;
    r.border_color = lv_color_white();
    r.border_width = 2;
    r.border_opa = LV_OPA_COVER;
    lv_canvas_draw_rect(canvas_, px - 5, py - 5, 10, 10, &r);
}

bool MapRenderer::isVisible(lv_coord_t px, lv_coord_t py) const {
    return px >= x_ && px < (width_ + x_) && py >= y_ && py < (height_ + y_);
}

void MapRenderer::drawHomeMarker(double lat, double lon) {
    lv_coord_t px, py;
    project(lat, lon, px, py);

    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = lv_color_hex(colHome_);
    dsc.width = 2;
    dsc.opa = LV_OPA_COVER;

    if (isVisible(px, py)) {
        lv_point_t p0{(lv_coord_t)(px - 4), (lv_coord_t)(py - 5)},
                   p1{(lv_coord_t)(px - 4), (lv_coord_t)(py + 5)};
        lv_canvas_draw_line(canvas_, &p0, 1, &dsc);

        lv_point_t p2{(lv_coord_t)(px + 4), (lv_coord_t)(py - 5)},
                   p3{(lv_coord_t)(px + 4), (lv_coord_t)(py + 5)};
        lv_canvas_draw_line(canvas_, &p2, 1, &dsc);

        lv_point_t p4{(lv_coord_t)(px - 4), (lv_coord_t)py},
                   p5{(lv_coord_t)(px + 4), (lv_coord_t)py};
        lv_canvas_draw_line(canvas_, &p4, 1, &dsc);
    }
}

void MapRenderer::panPx(int dx, int dy) {
    double scale = (double)tileSize_ * pow(2.0, zoom_);
    lon_ += dx / scale * 360.0;
    double ty = _latToTileY(lat_, zoom_) + (double)dy / tileSize_;
    lat_ = _tileYToLat(ty, zoom_);
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

const PixelBuffer* MapRenderer::_getTile(int z, int x, int y) {
    int oldestIdx = 0;
    uint32_t oldestTime = 0xFFFFFFFF;

    for (int i = 0; i < TILECACHE_SIZE; i++) {
        if (cache_[i].is(x,y,z)) {
            MAP_LOG("_getTile found %d/%d/%d", z,x,y);
            cache_[i].lastUsed = ++renderCount_;
            return &cache_[i].buffer;
        }
        if (cache_[i].lastUsed < oldestTime) {
            oldestTime = cache_[i].lastUsed;
            oldestIdx = i;
        }
    }
    MAP_LOG("_getTile missing %d/%d/%d, loading", z,x,y);
    return cache_[oldestIdx].load(x,y,z, pathPattern_, ++renderCount_);
}

void MapRenderer::_drawTiles() {
    double tx, ty;
    _latLonToTileF(lat_, lon_, zoom_, tx, ty);
    int cTx = (int)floor(tx), cTy = (int)floor(ty);
    int tOX = width_ / 2 - (int)((tx - cTx) * tileSize_);
    int tOY = height_ / 2 - (int)((ty - cTy) * tileSize_);
    MAP_LOG("_drawTiles cTx %d cTy %d", cTx, cTy);

    int tilesH = (int)ceil((double)width_ / tileSize_ / 2) + 1;
    int tilesV = (int)ceil((double)height_ / tileSize_ / 2) + 1;
    int nTiles = (int)pow(2.0, zoom_);

    for (int dy = -tilesV; dy <= tilesV; dy++) {
        for (int dx = -tilesH; dx <= tilesH; dx++) {
            MAP_LOG("_drawTiles dy %d dx %d", dy, dx);
            int px = tOX + dx * tileSize_;
            int py = tOY + dy * tileSize_;

            if (px + tileSize_ <= 0 || px >= width_) continue;
            if (py + tileSize_ <= 0 || py >= height_) continue;

            int tx2 = ((cTx + dx) % nTiles + nTiles) % nTiles;
            int ty2 = cTy + dy;

            if (ty2 < 0 || ty2 >= nTiles) {
                MAP_LOG("_drawTiles -> _drawNoTile dy %d dx %d", dy, dx);
                _drawNoTile(px, py);
                continue;
            }

            MAP_LOG("_drawTiles -> _getTile %d/%d/%d.png", zoom_, tx2, ty2);
            const PixelBuffer* pb = _getTile(zoom_, tx2, ty2);
            MAP_LOG("_drawTiles -> _getTile %d/%d/%d.png -> %p", zoom_, tx2, ty2, pb);
            if (pb) {
                MAP_LOG("_drawTiles -> copying %d/%d/%d.png to [%d, %d]", zoom_, tx2, ty2, px, py);
                pb->draw(canvas_, px, py);
                MAP_LOG("_drawTiles -> draw ok");
            } else {
                _drawNoTile(px, py);
            }
        }
    }
}

void MapRenderer::_drawNoTile(int px, int py) {
    lv_draw_rect_dsc_t r;
    lv_draw_rect_dsc_init(&r);
    r.bg_color = lv_color_hex(0x1a1f2a);
    r.bg_opa = LV_OPA_COVER;
    r.border_color = lv_color_hex(0x2a3040);
    r.border_width = 1;
    lv_canvas_draw_rect(canvas_, px, py, tileSize_, tileSize_, &r);
}
