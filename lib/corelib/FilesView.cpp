#include "FilesView.h"
#include "controller.h"
#include <SD.h>

void FilesView::create(lv_obj_t* parent, Controller* ctrl) {
    ctrl_ = ctrl;
    root_ = lv_obj_create(parent);
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);

    list_ = lv_list_create(root_);
    lv_obj_set_size(list_, LV_PCT(100), LV_PCT(100));
    lv_obj_center(list_);
}

void FilesView::show() {
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    _loadDir(currentDir_.c_str());
}

void FilesView::hide() {
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
}

void FilesView::update(bool inview) {}

void FilesView::onKey(uint8_t key) {
    if (key == ctrlbtns::KEY_RETURN) {
        lv_group_t* g = lv_group_get_default();
        lv_obj_t* btn = lv_group_get_focused(g);

        // Check that the focused object is actually one of our list buttons
        if (!btn || lv_obj_get_parent(btn) != list_) return;

        const char* text = lv_list_get_btn_text(list_, btn);
        if (strcmp(text, "..") == 0) {
            size_t pos = currentDir_.find_last_of('/', currentDir_.length() - 2);
            if (pos != std::string::npos) {
                currentDir_ = currentDir_.substr(0, pos + 1);
                _loadDir(currentDir_.c_str());
            }
        } else if (text[strlen(text)-1] == '/') {
            currentDir_ += text;
            _loadDir(currentDir_.c_str());
        } else if (strstr(text, ".gpx")) {
            if (ctrl_) {
                ctrl_->viewTrack_.load((currentDir_ + text).c_str());
                ctrl_->switchView(ViewID::MAP);
            }
        }
    } else if (key == ctrlbtns::KEY_ARROW_UP) {
        lv_group_focus_prev(lv_group_get_default());
    } else if (key == ctrlbtns::KEY_ARROW_DOWN) {
        lv_group_focus_next(lv_group_get_default());
    }
}

void FilesView::_loadDir(const char* path) {
    lv_obj_clean(list_);
    lv_group_t* g = lv_group_get_default();

    auto add_item = [&](const char* name) {
        lv_obj_t* btn = lv_list_add_btn(list_, NULL, name);
        if (g) lv_group_add_obj(g, btn);
    };

    add_item("..");

    File dir = SD.open(path);
    if (!dir) return;

    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;
        std::string name = entry.name();
        if (entry.isDirectory()) name += "/";
        add_item(name.c_str());
        entry.close();
    }
    dir.close();

    // Reset focus to the first item (the ".." button)
    lv_obj_t* first = lv_obj_get_child(list_, 0);
    if (first && g) lv_group_focus_obj(first);
}
