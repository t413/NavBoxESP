#pragma once
#include <stdint.h>

class Controller;
struct _lv_obj_t;

enum class ViewID {
    MAP   = 0,
    ABOUT = 1,
    COUNT
};

class ViewBase {
public:
    virtual ~ViewBase() = default;
    virtual void create(_lv_obj_t* parent, Controller*) = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void iterate(bool active) = 0;
    virtual void onKey(uint8_t key) = 0;
    virtual bool handleBack() = 0;
};
