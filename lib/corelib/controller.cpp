#include "controller.h"
#include <log.h>

Controller::Controller() {
}

void Controller::setup(lv_obj_t* parent) {
    gps_.begin(Serial1);

    // Initialize Map on the provided parent (the active screen)
    map_.begin(parent, 240, 135, "/osm/%d/%d/%d.png");
    map_.setZoom(14);
}

void Controller::iterate(uint32_t now) {
    if (gps_.iterate(now)) {
        auto loc = gps_.toPoint();
        if (map_.lat() == DEFAULT_LAT && loc.lat != 0.0) {
            map_.setHome(loc.lat, loc.lon);
            map_.setCenter(loc.lat, loc.lon);
            MAP_LOG("ctrl set initial center+home");
        } else {
            lv_coord_t px, py;
            map_.project(loc.lat, loc.lon, px, py);
            if (!map_.isVisible(px, py)) {
                map_.setCenter(loc.lat, loc.lon);
            }
        }
        MAP_LOG("ctrl draw <%5.5f,%5.5f>", loc.lat, loc.lon);
        map_.setDot(loc.lat, loc.lon);
    }
}
