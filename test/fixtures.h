#pragma once
#include <string>
#include <vector>
#include <lvgl.h>

std::string fmtstr(const char* fmt, ... );
std::string basename(const std::string& path);
std::string testname();

// A minimal 4x4 rainbow png. starts with r, g, b, w, then more rainbow for the rest.
extern const std::vector<uint8_t> png4x4;
extern const std::vector<uint8_t> png256hi;

struct TmpFileHelper {
    std::string fn_;
    TmpFileHelper(const std::vector<uint8_t> &img);
    ~TmpFileHelper() { rm(); }
    void rm();
};

struct LvglTestEnv {
    std::vector<lv_color_t> buf_;
    lv_disp_drv_t disp_drv_;
    lv_disp_draw_buf_t draw_buf_;
    lv_obj_t* canvas_ = nullptr;
    uint16_t width_ = 0, height_ = 0;

    LvglTestEnv(uint16_t width, uint16_t height);
    ~LvglTestEnv();
    void reset(uint16_t width=0, uint16_t height=0);
};
