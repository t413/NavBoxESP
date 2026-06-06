#include <gtest/gtest.h>
#include <views/SettingsView.h>
#include <ControllerBase.h>
#include <SettingsManager.h>
#include "fixtures.h"
#include <lvgl.h>

TEST(SettingsViewSimple, BasicShow) {
    fixtures::LvglTestEnv env(240, 135);
    SettingsManager mgr;
    int val = 10;
    String sval = "hey";

    // Group 0: Shown directly on main page
    auto g0 = mgr.group("basic");
    g0.add("Brightness", &val, 0, 100);

    // Group 1: Shown as a link on main page
    auto g1 = mgr.group("advanced");
    g1.add("Server", &sval);

    fixtures::MockCtrl ctrl(&mgr, &env);
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
    auto group = mgr.group("basic");
    group.add("zoom", &val, 0, 100).setAdjBump(1);

    fixtures::MockCtrl ctrl(&mgr, &env);
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
    val = 7;
    view.show(); //refresh
    view.onKey(ctrlbtns::KEY_ARROW_LEFT);
    EXPECT_EQ(val, 6);
    env.draw();
    env.save("_2edited");
}
