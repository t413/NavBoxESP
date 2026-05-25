#pragma once
#include <lvgl.h>

class Controller;

enum class ViewID {
    MAP,
    FILES,
    COUNT
};

class ViewBase {
public:
    virtual ~ViewBase() = default;
    virtual void create(lv_obj_t* parent, Controller*) = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void update(bool inview) = 0;
    virtual void onKey(uint8_t key) = 0;
    virtual bool handleBack() = 0;
};
