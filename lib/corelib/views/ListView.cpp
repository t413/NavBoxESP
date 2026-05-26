#include "ListView.h"
#include "../controller.h"

static constexpr uint32_t COL_BG      = 0x0D1117;
static constexpr uint32_t COL_ROW     = 0x21262D;
static constexpr uint32_t COL_ROW_SEL = 0x1F3A5F;
static constexpr uint32_t COL_BORDER  = 0x30363D;
static constexpr uint32_t COL_ACCENT  = 0x58A6FF;
static constexpr uint32_t COL_TEXT    = 0xE6EDF3;
static constexpr uint32_t COL_DIM     = 0x8B949E;

void ListView::create(lv_obj_t* parent, Controller* ctrl) {
    ctrl_ = ctrl;
    root_ = lv_obj_create(parent);
    lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root_, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_pad_all(root_, 4, 0);
    lv_obj_set_style_border_width(root_, 0, 0);
    lv_obj_set_scrollbar_mode(root_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);

    header_ = lv_obj_create(root_);
    lv_obj_set_size(header_, LV_PCT(100), 20);
    lv_obj_set_style_bg_color(header_, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_border_side(header_, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header_, lv_color_hex(COL_BORDER), 0);
    lv_obj_set_style_border_width(header_, 1, 0);
    lv_obj_set_style_pad_all(header_, 2, 0);

    headerLabel_ = lv_label_create(header_);
    lv_obj_set_align(headerLabel_, LV_ALIGN_CENTER);
    lv_obj_set_style_text_font(headerLabel_, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(headerLabel_, lv_color_hex(COL_ACCENT), 0);
    lv_label_set_text(headerLabel_, "");

    listCont_ = lv_obj_create(root_);
    lv_obj_set_width(listCont_, LV_PCT(100));
    lv_obj_set_flex_grow(listCont_, 1);
    lv_obj_set_style_bg_opa(listCont_, 0, 0);
    lv_obj_set_style_border_width(listCont_, 0, 0);
    lv_obj_set_flex_flow(listCont_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(listCont_, 3, 0);
    lv_obj_set_style_pad_all(listCont_, 0, 0);
}

void ListView::setHeader(const char* label) {
    lv_label_set_text(headerLabel_, label);
}

void ListView::addRow(const char* name, bool hasValue, bool isStatus) {
    lv_obj_t* row = lv_obj_create(listCont_);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_ver(row, 4, 0);
    lv_obj_set_style_pad_hor(row, 6, 0);
    lv_obj_set_style_radius(row, 2, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(COL_ROW), 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(COL_ROW_SEL), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(row, isStatus ? 2 : 1, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(COL_BORDER), 0);
    lv_obj_set_style_border_color(row, lv_color_hex(COL_ACCENT), LV_STATE_FOCUSED);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* nameLbl = lv_label_create(row);
    lv_label_set_text(nameLbl, name);
    lv_obj_set_style_text_font(nameLbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(nameLbl, lv_color_hex(COL_TEXT), 0);
    lv_obj_set_flex_grow(nameLbl, 1);

    lv_obj_t* valLbl = nullptr;
    if (hasValue) {
        valLbl = lv_label_create(row);
        lv_obj_set_style_text_font(valLbl, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(valLbl, lv_color_hex(COL_DIM), 0);
    }
    rows_.push_back({row, nameLbl, valLbl});
}

void ListView::setRowValue(int idx, const char* val, uint32_t color) {
    if (idx < 0 || idx >= rows_.size() || !rows_[idx].value) return;
    lv_label_set_text(rows_[idx].value, val);
    if (color != 0) lv_obj_set_style_text_color(rows_[idx].value, lv_color_hex(color), 0);
}

void ListView::setRowBorder(int idx, uint32_t color) {
    if (idx < 0 || idx >= rows_.size()) return;
    lv_obj_set_style_border_color(rows_[idx].container, lv_color_hex(color), 0);
}

void ListView::activateRow(int idx) {
    for (int i = 0; i < (int)rows_.size(); i++) {
        if (i == idx) {
            lv_obj_add_state(rows_[i].container, LV_STATE_FOCUSED);
            lv_obj_scroll_to_view(rows_[i].container, LV_ANIM_ON);
        } else {
            lv_obj_clear_state(rows_[i].container, LV_STATE_FOCUSED);
        }
    }
}

void ListView::show() { lv_obj_clear_flag(root_, LV_OBJ_FLAG_HIDDEN); activateRow(focused_); }
void ListView::hide() { lv_obj_add_flag(root_, LV_OBJ_FLAG_HIDDEN); }
void ListView::iterate(bool active) {
    // if (active) refreshAll();
}

void ListView::onKey(uint8_t key) {
    if (key == ctrlbtns::KEY_RETURN) {
        onRowAction(focused_);
    } else if (key == ctrlbtns::KEY_ARROW_UP || key == ctrlbtns::KEY_ARROW_DOWN) {
        int prev = focused_;
        if (key == ctrlbtns::KEY_ARROW_UP) focused_ = (focused_ > 0) ? focused_ - 1 : rows_.size() - 1;
        else focused_ = (focused_ < (int)rows_.size() - 1) ? focused_ + 1 : 0;
        if (prev != focused_) activateRow(focused_);
    } else if (key == ctrlbtns::KEY_ARROW_LEFT || key == ctrlbtns::KEY_ARROW_RIGHT) {
        onRowAdjust(focused_, key == ctrlbtns::KEY_ARROW_RIGHT);
    }
}
