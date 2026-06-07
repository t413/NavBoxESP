#pragma once
#include <stdint.h>

struct _lv_obj_t;
class ViewBase;
class SettingsManager;
class InputBase;
struct BaseTouchPoint;
struct TrackballDelta;

namespace ctrlbtns {
    static constexpr char KEY_ESC = '`';
    static constexpr char KEY_RETURN = '\n';
    static constexpr char KEY_DELETE = 0x2a;
    static constexpr char KEY_ARROW_UP = ';';
    static constexpr char KEY_ARROW_DOWN = '.';
    static constexpr char KEY_ARROW_LEFT = ',';
    static constexpr char KEY_ARROW_RIGHT = '/';
}

/**
 * Abstract base class for Controller.
 * Contains only what views need: overlay management, view switching, and basic utilities.
 * Platform-agnostic and independent of GPS/M5/Arduino.
 */
class ControllerBase {
public:
    virtual ~ControllerBase() = default;

    virtual _lv_obj_t* getOverlayRoot() = 0;
    virtual void setOverlay(ViewBase* overlay) = 0;

    virtual const SettingsManager* getSetMgr() const = 0;
    virtual SettingsManager* getSetMgr() = 0;
    virtual bool onKey(uint8_t key, uint32_t now) { return false; }
    virtual bool onTouch(const BaseTouchPoint&, uint32_t now) { return false; }
    virtual bool onScroll(const TrackballDelta&, uint32_t now) { return false; }

    virtual std::pair<uint16_t, uint16_t> getDispSize() const = 0;
    virtual float getBatt() const = 0; /// battery percentage
    virtual const char* gitVersion() const = 0;
    virtual void setBrightness(uint8_t) = 0;
    virtual void nextView() { }
};

class InputBase {
public:
    virtual ~InputBase() = default;
    virtual bool begin(ControllerBase* ctrl) { ctrl_ = ctrl; return true; }
    virtual bool iterate(uint32_t now) = 0; // Called every frame to poll/update
    virtual void end() = 0;  // Called before light sleep
    virtual void onSleep(bool sleeping) { }  // Can override for low-power behavior
    virtual bool setBrightness(uint8_t) { return false; }
protected:
    ControllerBase* ctrl_ = nullptr;
};

struct BaseTouchPoint {
    int16_t x = -1, y = -1;
    bool pressed = false;
};
struct TrackballDelta {
    int16_t dx = 0, dy = 0;
    bool pressed = false;
};

class BattManagerBase {
public:
    virtual ~BattManagerBase() = default;
    virtual void setup() = 0;
    virtual float getMV() const = 0;
    virtual float getMVtoPercent(uint16_t mv) const = 0;
};
