#pragma once
#include <lvgl.h>

class Controller;

enum class ViewID {
    MAP,
    TEST,
    FILES,
    COUNT
};

class ViewBase {
public:
    virtual ~ViewBase() = default;
    virtual void create(lv_obj_t* parent, Controller*) = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void update(bool inview) = 0;
    virtual void onKey(uint8_t key) = 0;
};

// A minimal test view to demonstrate switching
class TestView : public ViewBase {
public:
    void create(lv_obj_t* parent, Controller*) override {
        root_ = lv_obj_create(parent);
        lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(root_, lv_color_hex(0x000000), 0);
        lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t* label = lv_label_create(root_);
        lv_label_set_text(label, "TEST VIEW\n(ESC to return)");
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(label);
    }
    void show() override { lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN); }
    void hide() override { lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN); }
    void update(bool inview) override {}
    void onKey(uint8_t key) override {}
private:
    lv_obj_t* root_;
};
