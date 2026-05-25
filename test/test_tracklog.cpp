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
    TrackLog tl;
    TrackPoint p1(37.8, -122.2, 100.0);
    tl.beginRecording(100000);

    tl.addPoint(p1);
    EXPECT_EQ(tl.points().size(), 1);
    MAP_LOG("p1 <%0.5f,%0.5f>", p1.lat, p1.lon);

    // Point 5m away (should be rejected)
    const auto lowdist = tl.minDist_ / 2;
    TrackPoint p2 = p1.fromDistHeading(lowdist, 90.0);
    MAP_LOG("p2 <%0.5f,%0.5f>", p2.lat, p2.lon);
    EXPECT_NEAR(p1.distTo(p2),       lowdist, 0.1);
    EXPECT_NEAR(p1.approxDistTo(p2), lowdist, 0.1);
    tl.addPoint(p2);
    EXPECT_EQ(tl.points().size(), 1);

    // Point 15m away (should be accepted)
    TrackPoint p3 = p1.fromDistHeading(lowdist * 3, 90.0);
    tl.addPoint(p3);
    EXPECT_EQ(tl.points().size(), 2);
}

TEST(TrackLog, GPXLoadSave) {
    const char* testPath = "/tmp/test_track.gpx";
    TrackLog tl("/tmp");
    tl.beginRecording(100000);

    TrackPoint p1{37.8044, -122.2712, 10.0};
    p1.epoch = 1716610000; // Fixed timestamp
    tl.addPoint(p1);

    for (int i = 0; i < 50; i++) {
        const double dist = 15.0 + (i % 10);
        const double heading = (i * 13) % 360;
        TrackPoint p = p1.fromDistHeading(dist, heading);
        p.epoch = 1716610000 + (i * 10);
        tl.addPoint(p);
    }

    tl.stopRecording();
    MAP_LOG("RAW [%d] points -> [%d path]", tl.recordedPoints_, tl.points().size());
    EXPECT_EQ(tl.recordedPoints_, 51);

    // Verify file content manually
    std::ifstream ifs(testPath);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    EXPECT_TRUE(content.find("<trkpt lat=\"37.8044") != string::npos);
    EXPECT_TRUE(content.find("<time>2024-05-25T04:06:40Z</time>") != string::npos);

    // Test Loading
    TrackLog tl2;
    EXPECT_TRUE(tl2.load(testPath));
    EXPECT_EQ(tl2.points().size(), 2);
    EXPECT_NEAR(tl2.points()[0].lat, 37.8044, 0.0001);
}
