// FilesView.cpp
#include "FilesView.h"
#include "../controller.h"
#include <lvgl.h>
#include <SD.h>
#include <navboxlib/log.h>

void FilesView::create(lv_obj_t* parent, Controller* ctrl) {
    ListView::create(parent, ctrl);
}

void FilesView::show() {
    refresh();
    ListView::show();
}

void FilesView::iterate(bool inview) { }

void FilesView::onRowAction(int idx) {
    if (idx < 0 || idx >= (int)rows_.size()) return;
    const char* text = lv_label_get_text(rows_[idx].label);
    if (strcmp(text, "..") == 0) {
        handleBack();
    } else if (text[strlen(text)-1] == '/') {
        std::string next = currentDir_;
        if (next.back() != '/') next += "/";
        setDir(next + text);
    } else if (strstr(text, ".gpx")) {
        if (ctrl_) {
            showSpinner(true);
            std::string fullPath = currentDir_;
            if (fullPath.back() != '/') fullPath += "/";
            fullPath += text;
            if (onFileSelected_)
                onFileSelected_(fullPath.c_str());
            showSpinner(false);
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
    focused_ = 0;
    refresh();
}

void FilesView::refresh() {
    lv_obj_clean(listCont_);
    rows_.clear();
    focused_ = 0;

    setHeader(("Load GPX path: " + currentDir_).c_str());
    MAP_LOG("filesview loading %s", currentDir_.c_str());

    if (currentDir_ != "/") {
        addRow("..", "back", listc::COL_DIM);
    }

    File dir = SD.open(currentDir_.c_str());
    if (!dir) {
        MAP_LOG("filesview err opening path");
        return;
    }

    File entry;
    while (entry = dir.openNextFile()) {
        std::string name = entry.name();

        if (name[0] != '.') { // only show non-hidden stuff
            if (entry.isDirectory()) {
                addRow((name + "/").c_str(), "folder", listc::COL_DIM);
            } else {
                size_t dot = name.find_last_of('.');
                std::string ext = (dot != std::string::npos) ? name.substr(dot + 1) : "";
                auto color = (ext == "gpx") ? listc::COL_ACCENT : listc::COL_DIM;
                addRow(name.c_str(), ext.c_str(), color);
            }
        }
        entry.close();
    }
    dir.close();

    if (!rows_.empty()) activateRow(0);
}
