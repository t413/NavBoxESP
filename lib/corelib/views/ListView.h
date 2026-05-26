#pragma once
#include "../View.h"
#include <vector>

class ListView : public ViewBase {
public:
    virtual ~ListView() = default;
    void create(lv_obj_t* parent, Controller* ctrl) override;
    void show() override;
    void hide() override;
    void iterate(bool active) override;
    void onKey(uint8_t key) override;
    bool handleBack() override { return false; }

protected:
    virtual void onRowAction(int idx) = 0;
    virtual void onRowAdjust(int idx, bool right) {}
    virtual void refreshRow(int idx) {}
    virtual void refreshAll() { for (int i = 0; i < (int)rows_.size(); i++) refreshRow(i); }

    void setHeader(const char*);
    void addRow(const char* name, bool hasValue = true, bool isStatus = false);
    void setRowValue(int idx, const char* val, uint32_t color = 0);
    void setRowBorder(int idx, uint32_t color);

    lv_obj_t* root_ = nullptr;
    lv_obj_t* header_ = nullptr;
    lv_obj_t* headerLabel_ = nullptr;
    lv_obj_t* listCont_ = nullptr;
    Controller* ctrl_ = nullptr;

    struct RowUI {
        lv_obj_t* container;
        lv_obj_t* label;
        lv_obj_t* value;
    };
    std::vector<RowUI> rows_;
    int focused_ = 0;

    void activateRow(int idx);
};
