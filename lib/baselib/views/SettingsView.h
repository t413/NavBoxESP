#pragma once
#include "ListView.h"
#include <vector>
#include <string>

class Setting;

/**
 * Settings UI View - displays and edits application settings
 * Uses ListView for standard list UI, adds edit mode for values
 */
class SettingsView : public ListView {
public:
    void create(struct _lv_obj_t* parent, ControllerBase* ctrl) override;
    void show() override;
    void iterate(bool active) override;
    bool handleBack() override;

    void onRowAction(int idx) override;
    void onRowAdjust(int idx, bool right) override;
    void refreshRow(int idx) override;

    struct SettingRow {
        Setting* setting;
        int uiIdx;  // index in ListView rows_
    };

    std::vector<SettingRow> settings_;
    int editingIdx_ = -1;
    std::string editBuffer_;
    bool isDirty_ = false;

    void _populateSettings();
    void _startEdit(int idx);
    void _confirmEdit();
    void _cancelEdit();
    virtual void onKey(uint8_t key) override;
};
