#pragma once
#include <string>
#include <vector>
#include <lvgl.h>
#include <filesystem>
#include <ControllerBase.h>

namespace fixtures {

std::string fmtstr(const char* fmt, ... );
std::string basename(const std::string& path);
std::string testname();
std::filesystem::path cwd();
std::filesystem::path testOutdir();
void printfile(const char* fn);

void draw_lvgl_png(lv_disp_drv_t* drv, const char* path);

// A minimal 4x4 rainbow png. starts with r, g, b, w, then more rainbow for the rest.
extern const std::vector<uint8_t> png4x4;
extern const std::vector<uint8_t> png256hi;

struct TmpFileHelper {
    std::string fn_;
    TmpFileHelper(const std::vector<uint8_t> &img, std::string fn="");
    TmpFileHelper(const std::string &content, std::string fn="");
    ~TmpFileHelper() { rm(); }
    void rm();
};

struct LvglTestEnv {
    std::vector<lv_color_t> buf_;
    lv_disp_drv_t disp_drv_;
    lv_disp_draw_buf_t draw_buf_;
    lv_disp_t* disp_ = nullptr;
    lv_obj_t* base_ = nullptr;
    uint16_t width_ = 0, height_ = 0;

    LvglTestEnv(uint16_t width, uint16_t height, bool clearfirst = true);
    ~LvglTestEnv();
    void reset(uint16_t width=0, uint16_t height=0);

    void draw();
    static std::filesystem::path outdir();
    void save(std::string suffix="_canvas");
    void clearfiles();
};

struct MockCtrl : public ControllerBase {
    SettingsManager* mgr_;
    LvglTestEnv* env_;
    MockCtrl(SettingsManager* m, LvglTestEnv* env=nullptr) : mgr_(m), env_(env) {}
    _lv_obj_t* getOverlayRoot() override { return nullptr; }
    void setOverlay(ViewBase*) override {}
    const SettingsManager* getSetMgr() const override { return mgr_; }
    SettingsManager* getSetMgr() override { return mgr_; }
    virtual std::pair<uint16_t, uint16_t> getDispSize() const override { return std::make_pair(env_? env_->width_ : 0, env_? env_->height_ : 0); }
    virtual uint8_t getBatt() const override { return 80; }
    virtual const char* gitVersion() const override { return "mock"; }
    virtual void setBrightness(uint8_t) override { }
};

} //fixtures
