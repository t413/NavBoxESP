// FilesView.h
#pragma once
#include "views/ListView.h"
#include <string>
#include <vector>

class FilesView : public ListView {
public:
    void create(_lv_obj_t* parent, Controller* ctrl) override;
    void show() override;
    void iterate(bool inview) override;
    bool handleBack() override;

    void setDir(std::string path);
    void refresh();

protected:
    void onRowAction(int idx) override;

private:
    std::string currentDir_ = "/maps";
};
