#pragma once
#include <stdint.h>

class ControllerBase;
class SettingsManager;
struct BaseTouchPoint;
struct TrackballDelta;
struct _lv_obj_t;

enum class ViewID {
    MAP      = 0,
    ABOUT    = 1,
    SETTINGS = 2,
    COUNT
};

class ViewBase {
public:
    virtual ~ViewBase() = default;
    virtual void loadSettings(SettingsManager&mgr) { }
    virtual void create(_lv_obj_t* parent, ControllerBase*) = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void iterate(uint32_t now, bool active) = 0;

    virtual bool onKey(uint8_t key, uint32_t now) { return false; }
    virtual bool onTouch(const BaseTouchPoint&, uint32_t now) { return false; }
    virtual bool onScroll(const TrackballDelta&, uint32_t now) { return false; }
    virtual bool handleBack() = 0;
};
