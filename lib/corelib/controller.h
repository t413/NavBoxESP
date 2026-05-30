#pragma once
#include <stdint.h>
#include "GpsManager.h"
#include <navboxlib/TrackLog.h>
#include <ControllerBase.h>
#include <views/ViewBase.h>

class MapView;
struct _lv_obj_t;

#define BASEDIR_TILES "/maps/osm"
#define BASEDIR_TRACKS_REC "/maps/tracks"

class Controller : public ControllerBase {
public:
    Controller(const char* gitVersion);

    void setup(_lv_obj_t* parent);
    void iterate(uint32_t now);
    void switchView(ViewID id);
    _lv_obj_t* getOverlayRoot() override { return overlayRoot_; }
    void setOverlay(ViewBase*) override;
    SettingsManager* getSetMgr() const override { return nullptr; }
    void wakeup(uint32_t now);
    void doLightSleep();

    bool toggleRecording();
    bool isRecording() const { return recordTrack_.isRecording(); }
    bool loadTrack(const char* path, TrackLog*);

    uint8_t getBatt() const; /// battery percentage
    const char* gitVersion() const { return version_; }

private:
    MapView* getMapView();
    void _processKeys(uint32_t now);
    void _updateDimming(uint32_t now);
    _lv_obj_t* parent_  = nullptr;   // screen root
    _lv_obj_t* overlayRoot_ = nullptr; //for modal overlays
    ViewBase* overlay_ = nullptr;
    const char* version_ = "v?";
    uint32_t lastActivityMs_ = 0;
    bool dimmed_ = false, sleeping_ = false;

public:
    GpsManager gps_;
    TrackLog recordTrack_;
    TrackLog viewTrack_;
    ViewID currentView_ = ViewID::MAP;
    ViewBase* views_[(int)ViewID::COUNT] = {};
    uint32_t screenBrightness_ = 160, screenDimSec_ = 60;
};
