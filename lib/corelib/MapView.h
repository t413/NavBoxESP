#pragma once
#include "View.h"
#include "MapRenderer.h"

class Controller;
class GpsManager;
struct TrackPoint;

class MapView : public ViewBase {
public:
    void create(lv_obj_t*, Controller*) override;
    void show() override;
    void hide() override;
    void update(bool inview) override;
    void onGPSUpdate(GpsManager*);
    void onKey(uint8_t key) override;
    bool handleBack() override;

private:
    lv_obj_t* root_ = nullptr;
    Controller* ctrl_;
    MapRenderer map_;
    GpsManager* gps_;
    bool followMode_ = true;
    bool isActive_ = false;

    // Sidebar UI elements
    lv_obj_t* sidebar_ = nullptr;
    lv_obj_t* gpsDot_ = nullptr;
    lv_obj_t* battLabel_ = nullptr;
    lv_obj_t* satLabel_ = nullptr;
    lv_obj_t* speedLabel_ = nullptr;
    lv_obj_t* altLabel_ = nullptr;
    lv_obj_t* distLabel_ = nullptr;
    lv_obj_t* recDot_ = nullptr;
    lv_obj_t* recLabel_ = nullptr;
    lv_obj_t* viewDots_[(int)ViewID::COUNT] = {};

    void _createSidebar(lv_obj_t* parent);
    void _updateSidebar(const TrackPoint* = nullptr);

    // UI Helpers
    static lv_obj_t* _makeLabel(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, const lv_font_t* font);
    static lv_obj_t* _makeDot(lv_obj_t* parent, lv_coord_t x, lv_coord_t y);
    static void _makeDivider(lv_obj_t* parent, lv_coord_t y);
};
