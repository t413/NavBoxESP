#pragma once
#include <views/ViewBase.h>
#include <views/ListView.h>
#include <string>

class Controller;
struct TrackPoint;
class FilesView;
class TrackLog;

/// Second main view. Shows app info, GPX status, and system settings.
/// Subclasses ListView and uses the standard row interaction pattern.
class AboutView : public ListView {
public:
    void create(_lv_obj_t* parent, ControllerBase* ctrl) override;
    void show() override;
    bool handleBack() override { return false; }
    Controller* ctrlr() const { return (Controller*) ctrl_; }


private:
    void refresh();
    void onRowAction(int idx) override;
    void onRowAdjust(int idx, bool right) override;
    void refreshRow(int idx) override;
    void showFilePicker(TrackLog* dest);

    // Row indices — keep in sync with creation order
    enum RowID {
        ROW_VIEW_TRACK = 0,
        ROW_REC_TRACK,
        ROW_COUNT
    };

    static uint32_t statusBorderColor(bool active);
};
