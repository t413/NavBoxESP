#include <gtest/gtest.h>
#include <TrackLog.h>
#include <lvgl.h>
#include "fixtures.h"

using namespace std;

TEST(TrackLog, setup) {
    TrackLog tl;
    EXPECT_EQ(0, 0);
}

