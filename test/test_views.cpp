#include <gtest/gtest.h>
#include <views/SettingsView.h>
#include <ControllerBase.h>
#include <SettingsManager.h>
#include "fixtures.h"
#include <lvgl.h>

struct MockCtrl : public ControllerBase {
    SettingsManager* mgr;
    MockCtrl(SettingsManager* m) : mgr(m) {}
    _lv_obj_t* getOverlayRoot() override { return nullptr; }
    void setOverlay(ViewBase*) override {}
    const SettingsManager* getSetMgr() const override { return mgr; }
    SettingsManager* getSetMgr() override { return mgr; }
};

TEST(SettingsViewSimple, BasicShow) {
    fixtures::LvglTestEnv env(240, 135);
    SettingsManager mgr;
    int val = 10;
    String sval = "hey";
    mgr.add("test_val", &val, 0, 100);
    mgr.add("strval", &sval);

    MockCtrl ctrl(&mgr);
    SettingsView view;
    view.create(env.base_, &ctrl);
    view.show();

    env.draw();
    env.save("_basic");
}

TEST(SettingsViewSimple, EditValue) {
    fixtures::LvglTestEnv env(240, 135);
    SettingsManager mgr;
    int val = 50;
    mgr.add("zoom", &val, 0, 100);

    MockCtrl ctrl(&mgr);
    SettingsView view;
    view.create(env.base_, &ctrl);
    view.show();

    // Enter edit mode, type '7', confirm
    view.onKey(ctrlbtns::KEY_RETURN);
    view.onKey(ctrlbtns::KEY_DELETE);
    view.onKey(ctrlbtns::KEY_DELETE);
    view.onKey('7');
    env.draw();
    env.save("_1editing");
    view.onKey(ctrlbtns::KEY_RETURN);

    EXPECT_EQ(val, 7);
    view.onKey(ctrlbtns::KEY_ARROW_LEFT);
    EXPECT_EQ(val, 6);
    env.draw();
    env.save("_2edited");
}
