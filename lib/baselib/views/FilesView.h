// FilesView.h
#pragma once
#include "ListView.h"
#include <string>
#include <functional>

class FilesView : public ListView {
public:
    void create(_lv_obj_t* parent, ControllerBase* ctrl) override;
    void show() override;
    void iterate(uint32_t now, bool inview) override;
    bool handleBack() override;

    void setDir(std::string path);
    void setCallback(std::function<void(const char*)> cb) { onFileSelected_ = cb; }
    void refresh();

protected:
    void onRowAction(int idx) override;
    std::function<void(const char*)> onFileSelected_;

private:
    std::string currentDir_ = "/maps";
};
