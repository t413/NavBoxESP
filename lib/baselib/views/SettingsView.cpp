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
    lv_obj_set_width(listCont_, LV_PCT(96));
    lv_obj_set_flex_grow(listCont_, 1);
    lv_obj_set_style_bg_opa(listCont_, 0, 0);
    lv_obj_set_style_border_width(listCont_, 0, 0);
    lv_obj_set_flex_flow(listCont_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(listCont_, 3, 0);
    lv_obj_set_style_pad_hor(listCont_, 4, 0);
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
    if (isDirtyTime_ > 0 && (millis() - isDirtyTime_) > sc::FILE_WRITE_DELAY) {
        ctrl_->getSetMgr()->save();
        isDirtyTime_ = 0;
    }
}

void SettingsView::onKey(uint8_t key) {
    if (key == ctrlbtns::KEY_ARROW_UP) {
        focusedIdx_ = (focusedIdx_ > 0) ? focusedIdx_ - 1 : (int)rowdata_.size() - 1;
        _updateFocus();
    } else if (key == ctrlbtns::KEY_ARROW_DOWN) {
        if (currentGroup_ >= 0 && focusedIdx_ >= 0)
            _stopEdit(false); //cancel edit first
        focusedIdx_ = (focusedIdx_ < (int)rowdata_.size() - 1) ? focusedIdx_ + 1 : 0;
        _updateFocus();
    } else if (key == ctrlbtns::KEY_ARROW_LEFT || key == ctrlbtns::KEY_ARROW_RIGHT) {
        if (currentGroup_ >= 0 && focusedIdx_ >= 0) {
            _adjustValue(focusedIdx_, key == ctrlbtns::KEY_ARROW_RIGHT);
        }
    } else if (key == ctrlbtns::KEY_RETURN) {
        if (currentGroup_ < 0) {
            currentGroup_ = rowdata_[focusedIdx_].groupId;
            focusedIdx_ = 0;
            editingIdx_ = -1;
            _populate();
        } else if (editingIdx_ < 0) {
            _startEdit(focusedIdx_, true); // Enter starts edit at end
        } else {
            _stopEdit(true);
        }
    } else if (key == ctrlbtns::KEY_DELETE) {
        if (editingIdx_ >= 0 && editBuffer_.length()) {
            editBuffer_.pop_back();
            _refreshRowValue(editingIdx_);
        }
    } else if (key >= ' ' && key <= '~') {
        if (currentGroup_ >= 0) {
            if (editingIdx_ < 0)
                _startEdit(focusedIdx_, false); // Direct typing replaces contents
            editBuffer_ += (char)key;
            _refreshRowValue(editingIdx_);
        }
    }
}

bool SettingsView::handleBack() {
    if (editingIdx_ >= 0) {
        _stopEdit(false);
        return true;
    }
    if (currentGroup_ >= 0) {
        currentGroup_ = -1;
        focusedIdx_ = 0;
        editingIdx_ = -1;
        _populate();
        return true;
    }
    return false;
}

void SettingsView::_populate() {
    auto* mgr = ctrl_->getSetMgr();
    size_t rowIdx = 0;

    if (currentGroup_ < 0) {
        lv_label_set_text(headerLabel_, "Settings");
        auto& groups = mgr->getGroups();
        for (size_t i = 0; i < groups.size(); ++i) {
            _updateRow(rowIdx++, groups[i].c_str(), nullptr, (int)i);
        }
    } else {
        lv_label_set_text(headerLabel_, mgr->getGroup(currentGroup_).c_str());
        auto& settings = mgr->getSettings();
        for (auto& s : settings) {
            if (s->group_ == currentGroup_) {
                _updateRow(rowIdx++, s->key_.c_str(), s.get(), -1);
            }
        }
    }

    // Hide unused rows
    for (size_t i = rowIdx; i < rowdata_.size(); ++i) {
        lv_obj_add_flag(rowdata_[i].container, LV_OBJ_FLAG_HIDDEN);
    }

    _updateFocus();
}

void SettingsView::_updateRow(int idx, const char* name, Setting* s, int g) {
    if (idx >= (int)rowdata_.size()) {
        _addRow();
    }
    RowData& rd = rowdata_.at(idx);
    rd.setting = s;
    rd.groupId = g;
    lv_label_set_text(rd.label, name);
    _refreshRowValue(idx);
    lv_obj_clear_flag(rd.container, LV_OBJ_FLAG_HIDDEN);
}

void SettingsView::_addRow() {
    lv_obj_t* row = lv_obj_create(listCont_);
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

    lv_obj_t* nl = lv_label_create(row);
    lv_obj_set_style_text_font(nl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(nl, lv_color_hex(sc::COL_TEXT), 0);

    lv_obj_t* vl = lv_label_create(row);
    lv_obj_set_style_text_font(vl, &lv_font_montserrat_10, 0);

    RowData rd(nullptr, -1);
    rd.container = row;
    rd.label = nl;
    rd.value = vl;
    rowdata_.push_back(rd);
}

void SettingsView::_updateFocus() {
    for (int i = 0; i < (int)rowdata_.size(); ++i) {
        if (i == focusedIdx_) {
            lv_obj_set_style_bg_color(rowdata_[i].container, lv_color_hex(sc::COL_ROW_SEL), 0);
            lv_obj_scroll_to_view(rowdata_[i].container, LV_ANIM_ON);
        } else {
            lv_obj_set_style_bg_color(rowdata_[i].container, lv_color_hex(sc::COL_ROW), 0);
        }
    }
}

void SettingsView::_startEdit(int idx, bool append) {
    if (idx < 0 || idx >= (int)rowdata_.size()) return;
    Setting* s = rowdata_[idx].setting;
    if (!s) return;
    editingIdx_ = idx;
    editBuffer_ = append ? s->get().c_str() : "";
    lv_obj_set_style_border_color(rowdata_[idx].container, lv_color_hex(sc::COL_ACCENT), 0);
}

void SettingsView::_stopEdit(bool save) {
    if (editingIdx_ < 0) return;
    if (save) {
        Setting* s = rowdata_[editingIdx_].setting;
        s->set(editBuffer_.c_str());
        isDirtyTime_ = millis();
    }
    auto idx = editingIdx_; //backup
    editingIdx_ = -1;
    lv_obj_set_style_border_color(rowdata_[idx].container, lv_color_hex(sc::COL_BORDER), 0);
    _refreshRowValue(idx);
}

void SettingsView::_adjustValue(int idx, bool right) {
    if (idx < 0 || idx >= (int)rowdata_.size()) return;
    Setting* s = rowdata_[idx].setting;
    if (!s || !s->isNum_) return;

    float bump = s->getAdjBump();
    float current = s->get().toFloat();
    current += right ? bump : -bump;

    s->set(SetValue(current));
    isDirtyTime_ = millis();
    _refreshRowValue(idx);
}

void SettingsView::_refreshRowValue(int idx) {
    if (idx < 0 || idx >= (int)rowdata_.size()) return;

    std::string txt = "";
    uint32_t color = sc::COL_ACCENT;

    if (idx == editingIdx_) {
        txt = (editBuffer_.length() == 0) ? ".." : editBuffer_;
    } else {
        Setting* s = rowdata_[idx].setting;
        txt = s ? s->get().c_str() : ">";
        if (!s) color = sc::COL_DIM;
    }

    lv_label_set_text(rowdata_[idx].value, txt.c_str());
    lv_obj_set_style_text_color(rowdata_[idx].value, lv_color_hex(color), 0);
}
