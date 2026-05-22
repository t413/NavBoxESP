#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "GpsManager.h"
#include "MapRenderer.h"

class Controller {
public:
    Controller();

    void setup(lv_obj_t* parent);
    void iterate(uint32_t now);

public: //members
    GpsManager gps_;
    MapRenderer map_;
};
