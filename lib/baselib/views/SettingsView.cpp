#include "SettingsView.h"
#include "ControllerBase.h"
#include <SettingsManager.h>
#include <lvgl.h>
#include <navboxlib/log.h>
#include <cstring>

void SettingsView::create(struct _lv_obj_t* parent, ControllerBase* ctrl) {
    ListView::create(parent, ctrl);
    setHeader("Settings");
    _populateSettings();
}

void SettingsView::_populateSettings() {
    settings_.clear();
    lv_obj_clean(listCont_);
    auto mgr = ctrl_->getSetMgr();
    if (!mgr) return;
    const auto& settings = mgr->getSettings();

    for (auto& setting : settings) {
        if (!setting) continue;
        int uiIdx = addRow(setting->getKey().c_str(), setting->get().c_str());
        settings_.push_back(SettingRow{setting.get(), uiIdx});
    }
}

void SettingsView::show() {
    _populateSettings();
    ListView::show();
}

void SettingsView::iterate(bool active) {
    // Nothing special needed here
}

void SettingsView::onRowAction(int idx) {
    if (idx < 0 || idx >= (int)settings_.size()) return;
    if (idx == editingIdx_) {
        _confirmEdit();
    } else _startEdit(idx);
}

void SettingsView::onRowAdjust(int idx, bool right) {
    if (editingIdx_ >= 0) return; // Can't adjust while editing
    if (idx < 0 || idx >= (int)settings_.size()) return;

    auto setting = settings_[idx].setting;
    SetValue cur = setting->get();

    if (setting->isNum_) {
        float val = cur.toFloat();
        auto bump = setting->getAdjBump();
        val += (right? bump : -bump);
        setting->set(SetValue(val));
        refreshRow(idx);
        isDirty_ = true;
        if (ctrl_->getSetMgr()) ctrl_->getSetMgr()->save();
    }
}

void SettingsView::refreshRow(int idx) {
    if (idx < 0 || idx >= (int)settings_.size()) return;

    auto setting = settings_[idx].setting;
    SetValue val = setting->get();

    uint32_t color = listc::COL_ACCENT;
    if (editingIdx_ == idx) {
        color = listc::COL_BORDER; // Dim while editing
    }

    setRowValue(idx, val.c_str(), color);
}

void SettingsView::_startEdit(int idx) {
    if (idx < 0 || idx >= (int)settings_.size()) return;

    auto setting = settings_[idx].setting;
    editBuffer_ = setting->get().c_str();
    editingIdx_ = idx;
    MAP_LOG("setView:startEdit[%d] %s val %s", idx, setting->getKey().c_str(), editBuffer_.c_str());
    setRowBorder(idx, listc::COL_ACCENT);
}

void SettingsView::_confirmEdit() {
    if (editingIdx_ < 0 || editingIdx_ >= (int)settings_.size()) return;

    auto setting = settings_[editingIdx_].setting;
    setting->set(editBuffer_.c_str());
    isDirty_ = true;
    MAP_LOG("setView:edited[%d] %s val %s", editingIdx_, setting->getKey().c_str(), editBuffer_.c_str());
    if (ctrl_->getSetMgr()) ctrl_->getSetMgr()->save();

    int idx = editingIdx_;
    editingIdx_ = -1;
    editBuffer_.clear();

    refreshRow(idx);
}

void SettingsView::_cancelEdit() {
    if (editingIdx_ < 0) return;

    editingIdx_ = -1;
    editBuffer_.clear();
    refreshAll();
}

void SettingsView::onKey(uint8_t key) {
    if (editingIdx_ >= 0) {
        if (key == ctrlbtns::KEY_DELETE) {
            if (!editBuffer_.empty()) {
                editBuffer_.pop_back();
                setRowValue(editingIdx_, editBuffer_.empty() ? " " : editBuffer_.c_str(), listc::COL_ACCENT);
                MAP_LOG("setView:[%d]:del-char val %s", editingIdx_, editBuffer_.c_str());
            }
            return;
        } else if (key >= ' ' && key <= '~') {
            editBuffer_ += (char)key;
            setRowValue(editingIdx_, editBuffer_.c_str(), listc::COL_ACCENT);
            MAP_LOG("setView:[%d]:add-char val %s", editingIdx_, editBuffer_.c_str());
            return;
        }
    }
    ListView::onKey(key);
}

bool SettingsView::handleBack() {
    if (editingIdx_ >= 0) {
        _cancelEdit();
        return true; // Consumed
    }
    return false; // Not consumed, exit view
}
