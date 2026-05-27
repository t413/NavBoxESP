#include <gtest/gtest.h>
#include <TrackLog.h>
#include <lvgl.h>
#include <log.h>
#include "fixtures.h"
#include <fstream>

using namespace std;

TEST(TrackLog, setup) {
    TrackLog tl;
    EXPECT_EQ(tl.points().size(), 0);
    EXPECT_FALSE(tl.isRecording());
}

TEST(TrackLog, decimation) {
    TrackLog tl("/tmp");
    TrackPoint p1(37.8, -122.2, 100.0);
    tl.newRecording(1726600000);
    const char* testPath = tl.getRecPath();

    tl.addPoint(p1);
    EXPECT_EQ(tl.points().size(), 1);
    MAP_LOG("p1 <%0.5f,%0.5f>", p1.lat(), p1.lon());

    // Point 5m away (should be rejected)
    const auto lowdist = tl.minDist_ / 2;
    TrackPoint p2 = p1.fromDistHeading(lowdist, 90.0);
    MAP_LOG("p2 <%0.5f,%0.5f>", p2.lat(), p2.lon());
    EXPECT_NEAR(p1.distTo(p2),       lowdist, 0.1);
    EXPECT_NEAR(p1.approxDistTo(p2), lowdist, 0.1);
    tl.addPoint(p2);
    EXPECT_EQ(tl.points().size(), 1);

    // Point 15m away (should be accepted)
    TrackPoint p3 = p1.fromDistHeading(lowdist * 3, 90.0);
    tl.addPoint(p3);
    EXPECT_EQ(tl.points().size(), 2);
    tl.stopRecording();
    remove(testPath);
}

TEST(TrackLog, GPXLoadSave) {
    TrackLog tl("/tmp");
    tl.newRecording(1716610000);
    const char* testPath = tl.getRecPath();

    TrackPoint p1{37.8044, -122.2712, 10.0};
    p1.epoch = 1716610000; // Fixed timestamp
    tl.addPoint(p1);

    TrackPoint last = p1;
    for (int i = 0; i < 100; i++) {
        const double dist = 1.0 + (i % 10); // Spacing from 1 to 10m
        const double heading = 45.0 + (i % 20); // Consistent-ish heading
        TrackPoint p = last.fromDistHeading(dist, heading);
        p.alt = 100 + ((i % 10) * 10);
        p.epoch = 1716610000 + (i * 10);
        tl.addPoint(p);
        last = p;
    }

    tl.stopRecording();
    MAP_LOG("RAW [%d] points -> [%d path]", tl.recordedPoints_, (int)tl.points().size());
    EXPECT_EQ(tl.recordedPoints_, 101);

    // Verify Stats
    auto stats = tl.getStats();
    MAP_LOG("stats: dist=%0.1f, alt %0.1f, min %0.1f, gain %0.1f, loss %0.1f", stats.totalDist, stats.maxAltitude, stats.minAltitude, stats.totalElevGain, stats.totalElevLoss);
    EXPECT_GT(stats.totalDist, 250.0f);
    EXPECT_GT(stats.maxAltitude, 180.0f);

    // Verify file content manually
    std::ifstream ifs(testPath);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    EXPECT_TRUE(content.find("<trkpt lat=\"37.8044") != string::npos);
    EXPECT_TRUE(content.find("<time>2024-05-25T04:06:40Z</time>") != string::npos);

    // Test Loading
    TrackLog tl2;
    EXPECT_TRUE(tl2.load(testPath));
    EXPECT_GT(tl2.points().size(), 20);
    EXPECT_NEAR(tl2.points()[0].lat(), 37.8044, 0.0001);
    tl2.simplify();
    remove(testPath);
}

TEST(TrackLog, ResumeAndClear) {
    TrackLog tl("/tmp");

    // 1. Initial recording session
    tl.recordResume(1736610000);
    const auto fn = string(tl.getRecPath()); //keep a persistant copy
    tl.addPoint({37.0, -122.0, 100.0});
    tl.addPoint({37.001, -122.001, 110.0});
    tl.stopRecording();
    EXPECT_EQ(tl.recordedPoints_, 2);

    // 2. Resume recording (should append)
    tl.recordResume(1736610000);
    tl.addPoint({37.002, -122.002, 120.0});
    tl.stopRecording();
    EXPECT_GT(tl.getStats().totalDist, 100.0);

    // 4. Test Clear
    tl.clear();
    EXPECT_EQ(tl.points().size(), 0);
    EXPECT_EQ(tl.recordedPoints_, 0);
    EXPECT_EQ(tl.getStats().totalDist, 0);
    EXPECT_STREQ(tl.getRecPath(), "");
    EXPECT_FALSE(tl.isRecording());
    remove(fn.c_str());
}

TEST(TrackLog, simplify) {
    std::vector<GeoPoint> testPath = {
        {37.0, -122.0},
        {37.00001, -122.00001},  // very close, should cull
        {37.0001, -122.0001},
    };
    TrackLog::simplifyRadial(testPath, 10.0f); // keep only if >10m from neighbors
}
