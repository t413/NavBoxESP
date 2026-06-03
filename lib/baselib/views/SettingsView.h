#pragma once
#include "ViewBase.h"
#include <vector>
#include <memory>
#include <string>

class Setting;
class SettingsManager;
struct _lv_obj_t;

/**
 * Settings UI View - displays and edits application settings
 * Handles hierarchical groups and inline editing with type-to-replace behavior.
 */
class SettingsView : public ViewBase {
public:
    void create(struct _lv_obj_t* parent, ControllerBase* ctrl) override;
    void show() override;
    void hide() override;
    void iterate(bool active) override;
    void onKey(uint8_t key) override;
    bool handleBack() override;

private:
    struct RowData {
        Setting* setting = nullptr; // Null if this row is a Group
        int groupId = -1;          // Valid if this row is a Group
        _lv_obj_t* container = nullptr;
        _lv_obj_t* label = nullptr;
        _lv_obj_t* value = nullptr;
        RowData(Setting* s, int g) : setting(s), groupId(g) { }
    };

    ControllerBase* ctrl_ = nullptr;
    _lv_obj_t* root_ = nullptr;
    _lv_obj_t* headerLabel_ = nullptr;
    _lv_obj_t* listCont_ = nullptr;

    std::vector<RowData> rowdata_;

    int currentGroup_ = -1; // -1 means show group list
    int focusedIdx_ = 0;
    int editingIdx_ = -1;

    std::string editBuffer_;
    uint32_t isDirtyTime_ = 0;

    void _populate();
    void _addRow();
    void _updateRow(int idx, const char* name, Setting* s, int g);
    void _updateFocus();
    void _adjustValue(int idx, bool right);
    void _startEdit(int idx, bool append);
    void _confirmEdit(int idx);
    void _stopEdit(bool save);
    void _refreshRowValue(int idx);
};
