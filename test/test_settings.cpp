#include <gtest/gtest.h>
#include <SettingsManager.h>
#include <navboxlib/log.h>
#include <fstream>
#include "fixtures.h"

TEST(SettingsTest, ScalarPointerSettings) {
    SettingsManager mgr;
    int zoom = 5;
    float brightness = 0.5f;
    bool enabled = false;
    int changeCount = 0;

    auto& sZoom = mgr.add("zoom", &zoom, 1, 20);
    auto& sBright = mgr.add("bright", &brightness, 0.0f, 1.0f);
    auto& sEnabled = mgr.add("enabled", &enabled, false, true);

    sZoom.onChange([&]() { changeCount++; });

    // Test Initial Values
    EXPECT_EQ(sZoom.get().toInt(), 5);
    EXPECT_NEAR(sBright.get().toFloat(), 0.5f, 0.01f);
    EXPECT_EQ(sEnabled.get().toInt(), 0);

    // Test Valid Sets
    sZoom.set("10");
    EXPECT_EQ(zoom, 10);
    EXPECT_EQ(changeCount, 1);

    sBright.set("0.8");
    EXPECT_NEAR(brightness, 0.8f, 0.01f);

    sEnabled.set("true");
    EXPECT_TRUE(enabled);
    sEnabled.set("0");
    EXPECT_FALSE(enabled);

    // Test Bounds Enforcement
    sZoom.set("25"); // Over max
    EXPECT_EQ(zoom, 10);
    sZoom.set("0");  // Under min
    EXPECT_EQ(zoom, 10);
    EXPECT_EQ(changeCount, 1); // No change triggered for invalid sets

    sBright.set("2.0");
    EXPECT_NEAR(brightness, 0.8f, 0.01f);
}

TEST(SettingsTest, CallbackAndManagerLogic) {
    SettingsManager mgr;
    std::string url = "http://old.com";
    int internalVal = 100;

    // Add Callback Setting
    mgr.addFn("url",
        [&]() { return SetValue(url.c_str()); },
        [&](const SetValue& v) { url = v.c_str(); }
    );

    // Add Scalar Setting
    mgr.add("val", &internalVal, 0, 200);

    // Test find()
    Setting* sUrl = mgr.find("url");
    Setting* sVal = mgr.find("val");
    Setting* sNone = mgr.find("missing");

    ASSERT_NE(sUrl, nullptr);
    ASSERT_NE(sVal, nullptr);
    EXPECT_EQ(sNone, nullptr);

    // Test interaction via base class pointers
    EXPECT_EQ(sUrl->get(), "http://old.com");
    sUrl->set("http://new.com");
    EXPECT_EQ(url, "http://new.com");

    EXPECT_EQ(sVal->get().toInt(), 100);
    sVal->set("150");
    EXPECT_EQ(internalVal, 150);

    // Verify key retrieval
    EXPECT_EQ(sUrl->getKey(), "url");
    EXPECT_EQ(sVal->getKey(), "val");
}

TEST(SettingsTest, json) {
    const char* path = "/tmp/test.json";
    SettingsManager mgr(path);
    int zoom = 5;
    float brightness = 0.5f;
    bool enabled = false;
    String strval = "woo";

    mgr.add("zoom", &zoom, 1, 20);
    mgr.add("bright", &brightness, 0.0f, 1.0f);
    mgr.add("enabled", &enabled, false, true);
    mgr.add("strval", &strval);

    mgr.save();
    fixtures::printfile(path);

    //change all values
    zoom *= 2;
    brightness *= 2;
    enabled = true;
    strval = "boo";

    mgr.load(); //load from file, expect original values
    EXPECT_EQ(zoom, 5);
    EXPECT_NEAR(brightness, 0.5f, 0.01f);
    EXPECT_FALSE(enabled);
    EXPECT_EQ(strval, "woo");
    remove(path);
}

TEST(SettingsTest, PreserveUnknownValues) {
    fixtures::TmpFileHelper jfile("{\"zoom\": 12, \"extra_field\": \"keep_me\", \"bright\": 0.75}", "jsonfile");

    int zoom = 0;
    float bright = 0.0f;
    SettingsManager mgr(jfile.fn_.c_str());
    mgr.add("zoom", &zoom, 1, 20);
    mgr.add("bright", &bright, 0.0f, 1.0f);

    // 2. Load - should populate zoom and bright, ignore extra_field
    EXPECT_TRUE(mgr.load());
    EXPECT_EQ(zoom, 12);
    EXPECT_NEAR(bright, 0.75f, 0.01f);

    mgr.save();

    // Verify contents
    std::ifstream ifs(jfile.fn_);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    fixtures::printfile(jfile.fn_.c_str());
    EXPECT_TRUE(content.find("zoom") != std::string::npos);
    EXPECT_TRUE(content.find("extra_field") != std::string::npos);
}
