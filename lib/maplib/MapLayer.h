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
    virtual void update() = 0;
};

struct Marker {
    GeoPoint pos;
    uint16_t size;
    uint32_t color;
    char label = 0; //optional
    Marker() = default;
    Marker(const GeoPoint& p, uint16_t sz = 14, uint32_t col=0x24b9d7, char lbl = 0) : pos(p), size(sz), color(col), label(lbl) {}
};

class MarkerLayer : public MapLayer {
private:
    struct MarkObj { Marker m_; uint16_t id; _lv_obj_t* obj_; };
    MapRenderer* map_;
    std::vector<MarkObj> markers_;
    uint16_t count_ = 1337;

public:
    MarkerLayer(MapRenderer* map);
    ~MarkerLayer();

    uint16_t add(const Marker& m);
    bool updatePoint(uint16_t id, const GeoPoint &);
    const Marker& get(uint16_t id) const;
    void remove(uint16_t id);

    void update() override;
    void updateOne(MarkObj&);

    static const MarkObj empty;
};

class TrackLayer : public MapLayer {
private:
    MapRenderer* map_ = nullptr;
    TrackLog* track_ = nullptr;
    _lv_obj_t* line = nullptr;
    lv_point_t lvPoints[25];
    uint16_t pointCount = 0;
    bool visible_ = true;
public:
    TrackLayer(MapRenderer* map, uint32_t color, TrackLog* track);
    ~TrackLayer();
    void update() override;
};
