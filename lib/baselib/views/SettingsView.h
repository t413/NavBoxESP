#pragma once
#include "ViewBase.h"
#include <vector>
#include <memory>
#include <string>

class Setting;
class SettingsManager;
struct _lv_obj_t;
constexpr int HOMEPAGE_GROUP = -1;

/**
 * Settings UI View - displays and edits application settings
 * Handles hierarchical groups and inline editing with type-to-replace behavior.
 */
class SettingsView : public ViewBase {
public:
    void create(struct _lv_obj_t* parent, ControllerBase* ctrl) override;
    void show() override;
    void hide() override;
    void iterate(uint32_t now, bool active) override;
    bool onKey(uint8_t key, uint32_t now = 0) override;
    bool handleBack() override;

    void setGroup(int gid = HOMEPAGE_GROUP);
    uint8_t getRowCount() const { return visibleRows_; }
    void saveSettings();

private:
    struct RowData {
        Setting* setting_ = nullptr; // Null if this row is a Group
        int groupId_ = -1;          // Valid if this row is a Group
        _lv_obj_t* container_ = nullptr;
        _lv_obj_t* label_ = nullptr;
        _lv_obj_t* value_ = nullptr;
        RowData() = default;
        void create(struct _lv_obj_t* parent);
        void setup(Setting* s);
        void setup(std::string key, std::string value, int groupid);
        void setFocused(bool focused);
        void setActive(bool active);
        bool setHidden(bool hidden);
        void setValue(const char*, uint32_t color);
    };

    ControllerBase* ctrl_ = nullptr;
    _lv_obj_t* root_ = nullptr;
    _lv_obj_t* headerLabel_ = nullptr;
    _lv_obj_t* listCont_ = nullptr;

    std::vector<RowData> rowdata_;
    uint8_t visibleRows_ = 0;

    int currentGroup_ = HOMEPAGE_GROUP; /// what page is shown. home? settings group?
    int focusedIdx_ = 0; //what item is focused-on
    bool isEditing_ = false;

    std::string editBuffer_;
    uint32_t isDirtyTime_ = 0;
    bool autosaveEnabled_ = true;

    void _populate();
    void _addBlankRow();
    void _setupRow(int idx, Setting* s);
    void _setupRow(int idx, std::string key, int groupid);
    int _getAutosaveIdx() const;
    void _updateFocus();
    void _adjustValue(int idx, bool right);
    void _startEdit(int idx, bool append);
    void _confirmEdit(int idx);
    void _stopEdit(bool save);
    void _refreshRowValue(RowData& row, bool isEditing);
};
