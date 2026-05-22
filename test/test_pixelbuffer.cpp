#include <gtest/gtest.h>
#include <PixelBuffer.h>
#include <lvgl.h>
#include "fixtures.h"

using namespace std;

TEST(PixelBuffer, PixelBuffer_setup) {
    PixelBuffer pb;
    EXPECT_EQ(pb.width_ + pb.height_, 0);
    EXPECT_FALSE(pb.valid());
    EXPECT_TRUE(pb.allocate(20, 30));
    EXPECT_EQ(pb.width_, 20);
    EXPECT_EQ(pb.height_, 30);
    EXPECT_EQ(pb.size(), 600);
    EXPECT_TRUE(pb.valid());
    pb.clear();
    EXPECT_EQ(pb.width_ + pb.height_, 0);
    EXPECT_FALSE(pb.valid());
}

TEST(PixelBuffer, PixelBuffer_DrawAndGet) {
    PixelBuffer pb;
    pb.allocate(10, 10);
    pixel_t color = RGB(255, 128, 64);
    pb.drawPixelAbs(5, 5, color);

    pixel_t* ptr = pb.getPixelPtrAbs(5, 5);
    EXPECT_EQ(*ptr, color);

    // Check RGB macros
    EXPECT_NEAR(GET_RED(*ptr), 255, 8);   // 5-bit precision
    EXPECT_NEAR(GET_GREEN(*ptr), 128, 4); // 6-bit precision
    EXPECT_NEAR(GET_BLUE(*ptr), 64, 8);   // 5-bit precision
}

TEST(PixelBuffer, PixelBuffer_LoadImg) {

    fixtures::TmpFileHelper tf(fixtures::png4x4);

    PixelBuffer pb;
    EXPECT_TRUE(pb.loadImg(tf.fn_.c_str()));
    EXPECT_EQ(pb.width_, 4);
    EXPECT_EQ(pb.height_, 4);
    EXPECT_EQ(pb.size(), 16);

    EXPECT_EQ(pb.getData()[0], RGB(255, 0, 0));
    EXPECT_EQ(pb.getData()[1], RGB(0, 255, 0));
    EXPECT_EQ(pb.getData()[2], RGB(0, 0, 255));
    EXPECT_EQ(pb.getData()[3], RGB(255, 255, 255));
}



