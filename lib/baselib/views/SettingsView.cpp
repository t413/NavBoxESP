#include "SettingsView.h"
#include "../ControllerBase.h"
#include <SettingsManager.h>
#include <SettingTypes.h>
#include <lvgl.h>
#include <navboxlib/log.h>
#include <navboxlib/fileclass.h>

namespace sc { // Settings Constants
    static constexpr uint32_t COL_BG      = 0x0D1117;
    static constexpr uint32_t COL_ROW     = 0x21262D;
    static constexpr uint32_t COL_ROW_SEL = 0x1F3A5F;
    static constexpr uint32_t COL_BORDER  = 0x30363D;
    static constexpr uint32_t COL_ACCENT  = 0x58A6FF;
    static constexpr uint32_t COL_TEXT    = 0xE6EDF3;
    static constexpr uint32_t COL_DIM     = 0x8B949E;
    static constexpr uint32_t FILE_WRITE_DELAY = 5000;
}
constexpr int16_t AUTOSAVE_ID = -100;
constexpr int16_t EXIT_ID     = -101;


void SettingsView::create(struct _lv_obj_t* parent, ControllerBase* ctrl) {
    ctrl_ = ctrl;
    root_ = lv_obj_create(parent);
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root_, lv_color_hex(sc::COL_BG), 0);
    lv_obj_set_style_pad_all(root_, 4, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);

    _lv_obj_t* header = lv_obj_create(root_);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header, 0, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header, lv_color_hex(sc::COL_BORDER), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_pad_all(header, 2, 0);

    headerLabel_ = lv_label_create(header);
    lv_obj_set_align(headerLabel_, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(headerLabel_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(headerLabel_, lv_color_hex(sc::COL_ACCENT), 0);

    listCont_ = lv_obj_create(root_);
    lv_obj_set_width(listCont_, LV_PCT(100));
    lv_obj_set_flex_grow(listCont_, 1);
    lv_obj_set_style_bg_opa(listCont_, 0, 0);
    lv_obj_set_style_border_width(listCont_, 0, 0);
    lv_obj_set_flex_flow(listCont_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(listCont_, 6, 0);
    lv_obj_set_style_pad_hor(listCont_, 6, 0);
    lv_obj_set_align(listCont_, LV_ALIGN_TOP_MID);
}

void SettingsView::show() {
    _populate();
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
}

void SettingsView::hide() {
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
}

void SettingsView::iterate(bool active) {
    if (isDirtyTime_ > 0) {
        uint32_t elapsed = millis() - isDirtyTime_;
        if (autosaveEnabled_ && elapsed > sc::FILE_WRITE_DELAY) {
            saveSettings();
        }
        // Periodically refresh the autosave row if visible to show the countdown
        for (int i = 0; i < visibleRows_; i++)
            if (rowdata_.at(i).groupId_ == AUTOSAVE_ID)
                _refreshRowValue(rowdata_.at(i), false);
    }
}

void SettingsView::setGroup(int gid) {
    _stopEdit(false);
    currentGroup_ = gid;
    focusedIdx_ = 0;
    _populate();
}

bool SettingsView::onKey(uint8_t key, uint32_t now) {
    RowData& row = rowdata_.at(focusedIdx_ % visibleRows_);
    if (key == ctrlbtns::KEY_ARROW_UP || key == ctrlbtns::KEY_ARROW_DOWN) {
        if (!isEditing_) {
            focusedIdx_ = (focusedIdx_ + (key == ctrlbtns::KEY_ARROW_DOWN ? 1 : -1) + visibleRows_) % visibleRows_;
            _updateFocus();
        }
    } else if (key == ctrlbtns::KEY_ARROW_LEFT || key == ctrlbtns::KEY_ARROW_RIGHT) {
        _adjustValue(focusedIdx_, key == ctrlbtns::KEY_ARROW_RIGHT); //checks for setting
    } else if (key == ctrlbtns::KEY_RETURN) {
        if (row.setting_) {
            if (isEditing_) _stopEdit(true);
            else _startEdit(focusedIdx_, true);
        } else if (row.groupId_ >= 0) {
            MAP_LOG("group ENTER [%d]", row.groupId_);
            setGroup(row.groupId_);
        } else if (row.groupId_ == AUTOSAVE_ID) {
            MAP_LOG("toggle autosave ENTER");
            autosaveEnabled_ = !autosaveEnabled_;
            if (autosaveEnabled_) saveSettings();
            _refreshRowValue(row, false);
        } else if (row.groupId_ == EXIT_ID) {
            ctrl_->nextView();
        }
    } else if (key == ctrlbtns::KEY_DELETE) {
        if (isEditing_ && editBuffer_.length()) {
            editBuffer_.pop_back();
            _refreshRowValue(row, true);
        }
    } else if (key >= ' ' && key <= '~' && row.setting_) { //text entry!
        if (row.setting_->isNum_ && !((key >= '0' && key <= '9') || key == '.' || key == '-')) return false; //not a number
        if (!isEditing_)
            _startEdit(focusedIdx_, false); // Direct typing replaces contents
        editBuffer_ += (char)key;
        _refreshRowValue(row, true);
    } else return false;
    return true;
}

bool SettingsView::handleBack() {
    if (isEditing_) {
        _stopEdit(false); //esc hit while editing? cancel
    } else if (currentGroup_ >= 0) {
        setGroup(-1); //in a group? go home
    } else { return false; }
    return true;
}

void SettingsView::_populate() {
    auto* mgr = ctrl_->getSetMgr();
    size_t rowIdx = 0;

    if (currentGroup_ <= HOMEPAGE_GROUP) {
        lv_label_set_text(headerLabel_, "Settings");
        auto& settings = mgr->getSettings();
        // 1. add all settings from the first group (Group 0)
        for (auto& setting : settings)
            if (setting->group_ == 0)
                _setupRow(rowIdx++, setting.get());

        // 2. list all other groups
        auto& groups = mgr->getGroups();
        for (size_t i = 1; i < groups.size(); ++i)
            _setupRow(rowIdx++, groups[i], (int)i);

        // 3. Autosave button
        _setupRow(rowIdx++, "Autosave", AUTOSAVE_ID);

        // 3. exit button
        _setupRow(rowIdx++, "Exit", EXIT_ID);
    } else if (currentGroup_ >= 0) {
        std::string title = "Settings: " + mgr->getGroup(currentGroup_);
        lv_label_set_text(headerLabel_, title.c_str());
        auto& settings = mgr->getSettings();
        for (auto& setting : settings)
            if (setting->group_ == currentGroup_)
                _setupRow(rowIdx++, setting.get());
    }
    visibleRows_ = rowIdx;
    _updateFocus(); //hides/shows rows
}

void SettingsView::_addBlankRow() {
    rowdata_.push_back(RowData());
    rowdata_.back().create(listCont_);
}

void SettingsView::_setupRow(int idx, Setting* setting) {
    if (idx >= (int)rowdata_.size()) {
        _addBlankRow();
    }
    RowData& row = rowdata_.at(idx);
    row.setup(setting);
    _refreshRowValue(row, false);
}

void SettingsView::_setupRow(int idx, std::string key, int groupid) {
    if (idx >= (int)rowdata_.size()) {
        _addBlankRow();
    }
    RowData& row = rowdata_.at(idx);
    row.setup(key, ">", groupid);
    _refreshRowValue(row, false);
}

void SettingsView::_updateFocus() {
    for (int i = 0; i < rowdata_.size(); i++) {
        auto& row = rowdata_.at(i);
        if (row.setHidden(i >= visibleRows_)) continue; //skip the rest
        bool focused = (i == focusedIdx_);
        row.setFocused(focused);
        row.setActive(focused && isEditing_);
        if (focused) lv_obj_scroll_to_view(row.container_, LV_ANIM_ON);
    }
}

void SettingsView::_startEdit(int idx, bool append) {
    if (idx < 0 || idx >= visibleRows_) return;
    auto& row = rowdata_.at(focusedIdx_ % visibleRows_);
    if (!row.setting_) return;
    isEditing_ = true;
    editBuffer_ = append ? row.setting_->get().c_str() : "";
    _updateFocus();
}

void SettingsView::_stopEdit(bool save) {
    if (!isEditing_) return;
    auto& row = rowdata_.at(focusedIdx_ % visibleRows_);
    if (save && row.setting_) {
        MAP_LOG("set:stop [%d] -> %s (len %d)", focusedIdx_, editBuffer_.c_str(), (int)editBuffer_.length());
        String newv = editBuffer_.length() == 0 ? String() : String(editBuffer_.c_str());
        row.setting_->set(newv);
        isDirtyTime_ = millis();
    }
    isEditing_ = false;
    editBuffer_.clear();
    _updateFocus();
    _refreshRowValue(row, false);
}

void SettingsView::_adjustValue(int idx, bool right) {
    if (idx < 0 || idx >= visibleRows_) return;
    auto& row = rowdata_.at(focusedIdx_ % visibleRows_);
    if (!row.setting_ || !row.setting_->isNum_) return;

    float bump = row.setting_->getAdjBump();
    float current = row.setting_->get().toFloat();
    current += right ? bump : -bump;

    row.setting_->set(SetValue(current));
    isDirtyTime_ = millis();
    _refreshRowValue(row, false);
}

void SettingsView::_refreshRowValue(RowData& row, bool isEditing) {
    std::string txt = "";
    uint32_t color = sc::COL_ACCENT;

    if (isEditing) {
        txt = (editBuffer_.length() == 0) ? ".." : editBuffer_;
    } else if (row.groupId_ == AUTOSAVE_ID) { // Autosave row
        if (!autosaveEnabled_) {
            txt = "OFF";
            color = sc::COL_DIM;
        } else if (isDirtyTime_ == 0) {
            txt = "saved";
        } else {
            int sec = (int)((sc::FILE_WRITE_DELAY - (millis() - isDirtyTime_)) / 1000);
            txt = std::to_string(std::max(0, sec)) + "s";
        }
    } else { //setting or group
        txt = row.setting_ ? row.setting_->get().c_str() : ">";
        if (!row.setting_) color = sc::COL_DIM;
    }
    row.setValue(txt.c_str(), color);
}

void SettingsView::saveSettings() {
    ctrl_->getSetMgr()->save();
    isDirtyTime_ = 0;
}

void SettingsView::RowData::create(struct _lv_obj_t* parent) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_ver(row, 4, 0);
    lv_obj_set_style_pad_hor(row, 6, 0);
    lv_obj_set_style_radius(row, 2, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(row, lv_color_hex(sc::COL_ROW), 0);
    lv_obj_set_style_border_color(row, lv_color_hex(sc::COL_BORDER), 0);
    container_ = row;

    label_ = lv_label_create(row);
    lv_obj_set_style_text_font(label_, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(label_, lv_color_hex(sc::COL_TEXT), 0);

    value_ = lv_label_create(row);
    lv_obj_set_style_text_font(value_, &lv_font_montserrat_10, 0);
}

void SettingsView::RowData::setup(Setting* setting) {
    setting_ = setting;
    groupId_ = -1;
    lv_label_set_text(label_, setting->key_.c_str());
}

void SettingsView::RowData::setup(std::string key, std::string value, int groupid) {
    setting_ = nullptr;
    groupId_ = groupid;
    lv_label_set_text(label_, key.c_str());
    lv_label_set_text(value_, value.c_str());
}

void SettingsView::RowData::setFocused(bool focused) {
    lv_obj_set_style_bg_color(container_, lv_color_hex(focused? sc::COL_ROW_SEL : sc::COL_ROW), 0);
}
void SettingsView::RowData::setActive(bool active) {
    lv_obj_set_style_border_color(container_, lv_color_hex(active? sc::COL_ACCENT : sc::COL_BORDER), 0);
}
bool SettingsView::RowData::setHidden(bool hide) {
    if (hide) lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
    return hide;
}
void SettingsView::RowData::setValue(const char* value, uint32_t color) {
    lv_label_set_text(value_, value);
    lv_obj_set_style_text_color(value_, lv_color_hex(color), 0);
}
