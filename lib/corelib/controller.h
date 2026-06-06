#pragma once
#include <stdint.h>
#include "GpsManager.h"
#include <navboxlib/TrackLog.h>
#include <ControllerBase.h>
#include <SettingsManager.h>
#include <views/ViewBase.h>

class MapView;
struct _lv_obj_t;
#ifdef USE_LGFX
namespace lgfx { inline namespace v1 { class LGFX_Device; } }
#endif

#define BASEDIR_TILES "/maps/osm"
#define BASEDIR_TRACKS_REC "/maps/tracks"

class Controller : public ControllerBase {
public:
    Controller(const char* gitVersion);

    void setup(_lv_obj_t* parent);
#ifdef USE_LGFX
    void setupLgfx(lgfx::v1::LGFX_Device&);
#endif
    void setupGPS(int rx, int tx, uint32_t baud, HardwareSerial& uart);
    void iterate(uint32_t now);
    void switchView(ViewID id);
    _lv_obj_t* getOverlayRoot() override { return overlayRoot_; }
    void setOverlay(ViewBase*) override;
    const SettingsManager* getSetMgr() const override { return &settingsManager_; }
    SettingsManager* getSetMgr() override { return &settingsManager_; }
    void wakeup(uint32_t now);
    void doLightSleep();

    bool toggleRecording();
    bool isRecording() const { return recordTrack_.isRecording(); }
    bool loadTrack(const char* path, TrackLog*);

    virtual uint8_t getBatt() const override; /// battery percentage
    virtual void setBrightness(uint8_t) override;
    virtual const char* gitVersion() const override { return version_; }
    virtual std::pair<uint16_t, uint16_t> getDispSize() const override;

private:
    void loadSettings(SettingsManager& mgr);
    MapView* getMapView();
    void _processKeys(uint32_t now);
    void _updateDimming(uint32_t now);
    _lv_obj_t* parent_  = nullptr;   // screen root
    _lv_obj_t* overlayRoot_ = nullptr; //for modal overlays
    ViewBase* overlay_ = nullptr;
    const char* version_ = "v?";
    uint32_t lastActivityMs_ = 0;
    bool dimmed_ = false, sleeping_ = false;

#ifdef USE_LGFX
    lgfx::v1::LGFX_Device* lgfxDevice_ = nullptr;
#endif

public:
    SettingsManager settingsManager_;
    GpsManager gps_;
    TrackLog recordTrack_;
    TrackLog viewTrack_;
    ViewID currentView_ = ViewID::MAP;
    ViewBase* views_[(int)ViewID::COUNT] = {};
    uint8_t screenBrightness_ = 160;
    int screenDimSec_ = 60;
};
