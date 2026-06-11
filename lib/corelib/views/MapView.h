#pragma once
#include <views/ViewBase.h>
#include <navboxlib/MapRenderer.h>
#include <ControllerBase.h>
#include <stringmoc.h>

class Controller;
class GpsManager;
struct TrackPoint;
class TrackLayer;
struct _lv_obj_t;
struct _lv_font_t;

class MapView : public ViewBase {
public:
    void loadSettings(SettingsManager& mgr) override;
    void create(_lv_obj_t*, ControllerBase*) override;
    void show() override;
    void hide() override;
    void iterate(uint32_t now, bool inview) override;
    void onGPSUpdate(GpsManager*);
    bool onKey(uint8_t key, uint32_t now) override;
    bool onTouch(const BaseTouchPoint& delta, uint32_t now) override;
    bool onScroll(const TrackballDelta& delta, uint32_t now) override;
    bool handleBack() override;
    void setCenter(const GeoPoint &);
    MapRenderer& getMap() { return map_; }
    MarkerLayer& markerLayer();

private:
    _lv_obj_t* root_ = nullptr;
    Controller* ctrl_ = nullptr;
    MapRenderer map_;
    GpsManager* gps_ = nullptr;
    uint16_t dotMarkerId_ = 0;
    uint16_t homeMarkerId_ = 0;
    TrackLayer* recTrackLayer_ = nullptr;
    TrackLayer* viewTrackLayer_ = nullptr;
    bool followMode_ = true;
    bool isActive_ = false;
    uint32_t lastForceRedraw_ = 0;
    bool isPressed_ = false;
    BaseTouchPoint lastTouch_;

    // Sidebar UI elements
    _lv_obj_t* sidebar_ = nullptr;
    _lv_obj_t* gpsDot_ = nullptr;
    _lv_obj_t* battLabel_ = nullptr;
    _lv_obj_t* satLabel_ = nullptr;
    _lv_obj_t* speedLabel_ = nullptr;
    _lv_obj_t* altLabel_ = nullptr;
    _lv_obj_t* distLabel_ = nullptr;
    _lv_obj_t* recDot_ = nullptr;
    _lv_obj_t* recLabel_ = nullptr;
    _lv_obj_t* viewDots_[(int)ViewID::COUNT] = {};
    uint32_t sidebarBg_ = 0x161b28; // #040916

    void _createSidebar(_lv_obj_t* parent);
    void _updateSidebar(const TrackPoint* = nullptr);

    // UI Helpers
    static _lv_obj_t* _makeLabel(_lv_obj_t* parent, lv_coord_t x, lv_coord_t y, const _lv_font_t* font);
    static _lv_obj_t* _makeDot(_lv_obj_t* parent, lv_coord_t x, lv_coord_t y);
    static void _makeDivider(_lv_obj_t* parent, lv_coord_t y);
};

GeoPoint parseGeoPoint(const String&);
String geoPointToStr(const GeoPoint&);
