#include "AboutView.h"
#include "../controller.h"
#include <log.h>
#include <M5Unified.h>
#include <lvgl.h>

static constexpr uint32_t COL_GREEN = 0x3FB950;
static constexpr uint32_t COL_RED   = 0xFF7B72;
static constexpr uint32_t COL_ACCENT = 0x58A6FF;
static constexpr uint32_t COL_BORDER = 0x30363D;
static constexpr uint32_t COL_DIM    = 0x8B949E;

uint32_t AboutView::statusBorderColor(bool active) {
    return active ? COL_GREEN : COL_BORDER;
}

void AboutView::create(lv_obj_t* parent, Controller* ctrl) {
    ListView::create(parent, ctrl);

    // adjust header section
    lv_obj_set_height(header_, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(header_, LV_FLEX_FLOW_COLUMN);

    setHeader("NavBox by t413");

    lv_obj_t* versLabel = lv_label_create(header_);
    lv_label_set_text(versLabel, ctrl_->gitVersion());
    lv_obj_set_style_text_font(versLabel, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(versLabel, lv_color_hex(COL_DIM), 0);
    lv_obj_set_width(versLabel, LV_PCT(100));
    lv_obj_set_style_text_align(versLabel, LV_TEXT_ALIGN_CENTER, 0);

    // Add rows using ListView's addRow
    addRow("View Track", "");
    addRow("Rec Track", "");
    addRow("Brightness", "");
    addRow("Dim Time", "");

    // Initial refresh to populate all row values
    refreshAll();
}

void AboutView::show() {
    refresh();
    ListView::show();
}

void AboutView::iterate(bool inview) {
    ListView::iterate(inview);
}

void AboutView::refresh() {
    refreshAll();
}

void AboutView::refreshRow(int idx) {
    char buf[48];

    if (idx == ROW_VIEW_TRACK) {
        bool has = ctrl_->viewTrack_.size() > 0;
        setRowBorder(idx, statusBorderColor(has));
        const char* path = has ? ctrl_->viewTrack_.getRecPath() : "(none)";
        const char* slash = strrchr(path, '/');
        setRowValue(idx, slash ? slash + 1 : path);
    }
    else if (idx == ROW_REC_TRACK) {
        bool has = ctrl_->recordTrack_.size() > 0 || ctrl_->isRecording();
        setRowBorder(idx, statusBorderColor(has));
        if (ctrl_->isRecording()) {
            snprintf(buf, sizeof(buf), "REC %u pts", ctrl_->recordTrack_.recordedPoints_);
            setRowValue(idx, buf, COL_RED);
        } else {
            const char* path = has ? ctrl_->recordTrack_.getRecPath() : "(none)";
            const char* slash = strrchr(path, '/');
            setRowValue(idx, slash ? slash + 1 : path);
        }
    }
    else if (idx == ROW_BRIGHTNESS) {
        int pct = (int)roundf(ctrl_->screenBrightness_ * 100.0f / 255.0f);
        snprintf(buf, sizeof(buf), "%d%%  %s", pct, "<>");
        setRowValue(idx, buf);
    }
    else if (idx == ROW_DIM_TIME) {
        snprintf(buf, sizeof(buf), "%ds  %s", ctrl_->screenDimSec_, "<>");
        setRowValue(idx, buf);
    }
}

void AboutView::onRowAction(int idx) {
    switch (idx) {
        case ROW_VIEW_TRACK:
            if (ctrl_->viewTrack_.size() > 0) {
                ctrl_->viewTrack_.clear();
            }
            break;

        case ROW_REC_TRACK:
            if (ctrl_->isRecording()) {
                ctrl_->toggleRecording();
            } else if (ctrl_->recordTrack_.size() > 0) {
                ctrl_->recordTrack_.clear();
            }
            break;

        case ROW_BRIGHTNESS:
            ctrl_->screenBrightness_ = std::min(255U, ctrl_->screenBrightness_ + BRIGHT_STEP);
            M5.Display.setBrightness(ctrl_->screenBrightness_);
            break;

        case ROW_DIM_TIME: break; //require left/right adjust
        default:
            break;
    }
    refreshAll();
}

void AboutView::onRowAdjust(int idx, bool right) {
    char buf[48];

    if (idx == ROW_BRIGHTNESS) {
        int delta = right ? BRIGHT_STEP : -BRIGHT_STEP;
        int newVal = (int)ctrl_->screenBrightness_ + delta;
        ctrl_->screenBrightness_ = std::max((unsigned)BRIGHT_MIN, std::min(255U, (unsigned)newVal));
        M5.Display.setBrightness(ctrl_->screenBrightness_);
    }
    else if (idx == ROW_DIM_TIME) {
        ctrl_->screenDimSec_ = std::max((uint32_t)0, ctrl_->screenDimSec_ + (right ? DIM_STEP : -DIM_STEP));
    }
    refreshAll();
}
