#pragma once
#include <stdint.h>
#include <vector>
#include <misc/lv_area.h>
#include "GeoPoint.h"

class MapRenderer;
class TrackLog;
struct _lv_obj_t;

class MapLayer {
public:
    virtual ~MapLayer() = default;
    virtual void update(MapRenderer* renderer) = 0;
    virtual void setVisible(bool visible) = 0;
    virtual bool isVisible() const = 0;
};

struct Marker {
    GeoPoint pos;
    uint16_t size = 14;
    uint32_t color = 0x24b9d7; // #24b9d7
    char label = 0; //optional
};

class MarkerLayer : public MapLayer {
private:
    _lv_obj_t* parent_;
    std::vector<Marker> markers_;
    std::vector<_lv_obj_t*> objs_;
    bool visible_ = true;

public:
    MarkerLayer(_lv_obj_t* parent);
    ~MarkerLayer();

    Marker* addMarker(const Marker& m);
    void removeMarker(size_t idx);
    void update(MapRenderer* renderer) override;
    void setVisible(bool v) override { visible_ = v; }
    bool isVisible() const override { return visible_; }
};

class TrackLayer : public MapLayer {
private:
    TrackLog* track_;
    _lv_obj_t* line = nullptr;
    lv_point_t lvPoints[25];
    uint16_t pointCount = 0;
    bool visible_ = true;
public:
    TrackLayer(_lv_obj_t* parent, uint32_t color, TrackLog* track);
    ~TrackLayer();
    void update(MapRenderer* renderer) override;
    void setVisible(bool v) override { visible_ = v; }
    bool isVisible() const override { return visible_; }
};
