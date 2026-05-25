// FilesView.cpp
#include "FilesView.h"
#include "controller.h"
#include <SD.h>
#include <log.h>

void FilesView::create(lv_obj_t* parent, Controller* ctrl) {
    ctrl_ = ctrl;
    root_ = lv_obj_create(parent);
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(root_, 0, 0);
    lv_obj_set_style_pad_row(root_, 0, 0);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);

    pathstr_ = lv_label_create(root_);
    lv_obj_set_width(pathstr_, LV_PCT(100));
    lv_obj_set_style_text_align(pathstr_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(pathstr_, lv_color_hex(0x2a3040), 0);
    lv_obj_set_style_bg_opa(pathstr_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(pathstr_, 2, 0);

    // Create a scrollable container instead of lv_list
    container_ = lv_obj_create(root_);
    lv_obj_set_width(container_, LV_PCT(100));
    lv_obj_set_flex_grow(container_, 1);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(container_, LV_DIR_VER);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_pad_row(container_, 2, 0);  // Space between rows in flex layout
}

void FilesView::show() {
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN);
    refresh();
}

void FilesView::hide() {
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
}

void FilesView::update(bool inview) {}

void FilesView::onKey(uint8_t key) {
    lv_obj_t* first_btn = lv_obj_get_child(container_, 0);
    if (!first_btn) return;

    int child_count = 0;
    lv_obj_t* child = lv_obj_get_child(container_, 0);
    while (child) {
        child_count++;
        child = lv_obj_get_child(container_, child_count);
    }

    if (key == ctrlbtns::KEY_RETURN) {
        lv_obj_t* focused = lv_obj_get_child(container_, focused_index_);
        if (!focused) return;
        const char* text = lv_label_get_text(lv_obj_get_child(focused, 0));

        if (strcmp(text, "..") == 0) {
            handleBack();
        } else if (text[strlen(text)-1] == '/') {
            std::string next = currentDir_;
            if (next.back() != '/') next += "/";
            setDir(next + text);
        } else if (strstr(text, ".gpx")) {
            if (ctrl_) {
                std::string fullPath = currentDir_;
                if (fullPath.back() != '/') fullPath += "/";
                fullPath += text;
                ctrl_->loadTrack(fullPath.c_str());
            }
        }
    } else if (key == ctrlbtns::KEY_ARROW_UP) {
        lv_obj_t* prev_btn = lv_obj_get_child(container_, focused_index_);
        if (prev_btn) lv_obj_clear_state(prev_btn, LV_STATE_PRESSED);

        focused_index_ = (focused_index_ > 0) ? focused_index_ - 1 : child_count - 1;
        lv_obj_t* btn = lv_obj_get_child(container_, focused_index_);
        if (btn) {
            lv_obj_add_state(btn, LV_STATE_PRESSED);
            lv_obj_scroll_to_view(btn, LV_ANIM_OFF);
        }
    } else if (key == ctrlbtns::KEY_ARROW_DOWN) {
        lv_obj_t* prev_btn = lv_obj_get_child(container_, focused_index_);
        if (prev_btn) lv_obj_clear_state(prev_btn, LV_STATE_PRESSED);

        focused_index_ = (focused_index_ < child_count - 1) ? focused_index_ + 1 : 0;
        lv_obj_t* btn = lv_obj_get_child(container_, focused_index_);
        if (btn) {
            lv_obj_add_state(btn, LV_STATE_PRESSED);
            lv_obj_scroll_to_view(btn, LV_ANIM_OFF);
        }
    }
}

bool FilesView::handleBack() {
    if (currentDir_ == "/")
        return false;
    size_t pos = currentDir_.find_last_of('/');
    std::string parent = (pos == 0) ? "/" : currentDir_.substr(0, pos);
    setDir(parent);
    return true;
}

void FilesView::setDir(std::string path) {
    if (path.length() > 1 && path.back() == '/' && path != "/") {
        path.pop_back(); //strip trailing /
    }
    currentDir_ = path;
    focused_index_ = 0;
    refresh();
}

void FilesView::refresh() {
    lv_obj_clean(container_);
    const auto cstr = currentDir_.c_str();
    lv_label_set_text(pathstr_, cstr);
    focused_index_ = 0;
    MAP_LOG("filesview loading %s", cstr);

    auto add_item = [&](const char* name) {
        lv_obj_t* btn = lv_btn_create(container_);
        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_set_height(btn, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(btn, 4, 0);  // Reduced padding

        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, name);
        lv_obj_center(label);
    };

    add_item("..");

    File dir = SD.open(cstr);
    if (!dir) {
        MAP_LOG("filesview err opening path");
        return;
    }

    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        std::string name = entry.name();

        if (name[0] != '.') { // only show non-hidden stuff
            if (entry.isDirectory()) name += "/";
            add_item(name.c_str());
        }
        entry.close();
    }
    dir.close();

    // Highlight the first item
    lv_obj_t* first = lv_obj_get_child(container_, 0);
    if (first) lv_obj_add_state(first, LV_STATE_PRESSED);
}
