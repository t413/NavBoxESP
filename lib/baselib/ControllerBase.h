#pragma once
#include <stdint.h>

struct _lv_obj_t;

namespace ctrlbtns {
    static constexpr char KEY_ESC = '`';
    static constexpr char KEY_RETURN = '\n';
    static constexpr char KEY_DELETE = 0x2a;
    static constexpr char KEY_ARROW_UP = ';';
    static constexpr char KEY_ARROW_DOWN = '.';
    static constexpr char KEY_ARROW_LEFT = ',';
    static constexpr char KEY_ARROW_RIGHT = '/';
}

// Forward declaration
class ViewBase;
class SettingsManager;

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

    virtual std::pair<uint16_t, uint16_t> getDispSize() const = 0;
    virtual uint8_t getBatt() const = 0; /// battery percentage
    virtual const char* gitVersion() const = 0;
    virtual void setBrightness(uint8_t) = 0;
};
