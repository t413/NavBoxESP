#pragma once
#include "../View.h"
#include "ListView.h"
#include <string>

class Controller;
struct TrackPoint;

/// Second main view. Shows app info, GPX status, and system settings.
/// Subclasses ListView and uses the standard row interaction pattern.
class AboutView : public ListView {
public:
    void create(_lv_obj_t* parent, Controller* ctrl) override;
    void show() override;
    void iterate(bool inview) override;
    bool handleBack() override { return false; }

private:
    void refresh();
    void onRowAction(int idx) override;
    void onRowAdjust(int idx, bool right) override;
    void refreshRow(int idx) override;

    // Row indices — keep in sync with creation order
    enum RowID {
        ROW_VIEW_TRACK = 0,
        ROW_REC_TRACK,
        ROW_BRIGHTNESS,
        ROW_DIM_TIME,
        ROW_COUNT
    };

    static constexpr uint8_t BRIGHT_STEP = 20;
    static constexpr uint8_t BRIGHT_MIN  = 20;

    static const uint32_t DIM_STEP = 10;
    static uint32_t statusBorderColor(bool active);
};
