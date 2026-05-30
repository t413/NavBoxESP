#pragma once
#include "ViewBase.h"
#include <vector>

class ListView : public ViewBase {
public:
    virtual ~ListView() = default;
    void create(_lv_obj_t* parent, ControllerBase* ctrl) override;
    void show() override;
    void hide() override;
    void iterate(bool active) override;
    void onKey(uint8_t key) override;
    virtual bool handleBack() override { return false; }

protected:
    virtual void onRowAction(int idx) = 0;
    virtual void onRowAdjust(int idx, bool right) {}
    virtual void refreshRow(int idx) {}
    virtual void refreshAll() { for (int i = 0; i < (int)rows_.size(); i++) refreshRow(i); }

    void setHeader(const char*);
    int addRow(const char* name, const char* value, uint32_t valColor=0);
    void setRowValue(int idx, const char* val, uint32_t color = 0);
    void setRowBorder(int idx, uint32_t color);
    void showSpinner(bool show);

    _lv_obj_t* root_ = nullptr;
    _lv_obj_t* header_ = nullptr;
    _lv_obj_t* headerLabel_ = nullptr;
    _lv_obj_t* listCont_ = nullptr;
    _lv_obj_t* spinner_ = nullptr;
    ControllerBase* ctrl_ = nullptr;
    friend class AboutView;

    struct RowUI {
        _lv_obj_t* container;
        _lv_obj_t* label;
        _lv_obj_t* value;
    };
    std::vector<RowUI> rows_;
    int focused_ = 0;

    void activateRow(int idx);
};

namespace listc {
    static constexpr uint32_t COL_BG      = 0x0D1117;
    static constexpr uint32_t COL_ROW     = 0x21262D;
    static constexpr uint32_t COL_ROW_SEL = 0x1F3A5F;
    static constexpr uint32_t COL_BORDER  = 0x30363D;
    static constexpr uint32_t COL_ACCENT  = 0x58A6FF;
    static constexpr uint32_t COL_TEXT    = 0xE6EDF3;
    static constexpr uint32_t COL_DIM     = 0x8B949E;
}
