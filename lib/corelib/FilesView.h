#pragma once
#include "View.h"
#include <string>
#include <vector>

class FilesView : public ViewBase {
public:
    void create(lv_obj_t* parent, Controller* ctrl) override;
    void show() override;
    void hide() override;
    void update(bool inview) override;
    void onKey(uint8_t key) override;

private:
    void _loadDir(const char* path);

    lv_obj_t* root_ = nullptr;
    lv_obj_t* list_ = nullptr;
    Controller* ctrl_ = nullptr;
    std::string currentDir_ = "/maps/";
};
