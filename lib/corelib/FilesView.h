// FilesView.h
#pragma once
#include "View.h"
#include <string>
#include <vector>

class FilesView : public ViewBase {
public:
    void create(lv_obj_t* parent, Controller* ctrl) override;
    void show() override;
    void hide() override;
    void iterate(bool inview) override;
    void onKey(uint8_t key) override;
    bool handleBack() override;

    void setDir(std::string path);
    void refresh();

private:
    static void _btn_event_cb(lv_event_t* e);

    lv_obj_t* root_ = nullptr;
    lv_obj_t* pathstr_ = nullptr;
    lv_obj_t* container_ = nullptr;  // Changed from list_
    Controller* ctrl_ = nullptr;
    std::string currentDir_ = "/maps";
    int focused_index_ = 0;
};
