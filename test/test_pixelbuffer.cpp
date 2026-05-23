#include <gtest/gtest.h>
#include <PixelBuffer.h>
#include <lvgl.h>
#include "fixtures.h"

using namespace std;

TEST(PixelBuffer, setup) {
    PixelBuffer pb;
    EXPECT_EQ(pb.width_ + pb.height_, 0);
    EXPECT_FALSE(pb.valid());
    EXPECT_TRUE(pb.allocate(20, 30));
    EXPECT_EQ(pb.width_, 20);
    EXPECT_EQ(pb.height_, 30);
    EXPECT_EQ(pb.size(), 600);
    EXPECT_TRUE(pb.valid());
    EXPECT_EQ(pb.getOffsetX() + pb.getOffsetY(), 0);
    EXPECT_FALSE(pb.isSparse());
    pb.clear();
    EXPECT_EQ(pb.width_ + pb.height_, 0);
    EXPECT_FALSE(pb.valid());
}

TEST(PixelBuffer, drawAndGet) {
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

TEST(PixelBuffer, loadImg) {

    fixtures::TmpFileHelper tf(fixtures::png4x4);

    PixelBuffer pb;
    EXPECT_TRUE(pb.loadImg(tf.fn_.c_str(), Bounds()));
    EXPECT_EQ(pb.width_, 4);
    EXPECT_EQ(pb.height_, 4);
    EXPECT_EQ(pb.size(), 16);

    EXPECT_EQ(pb.getData()[0], RGB(255, 0, 0));
    EXPECT_EQ(pb.getData()[1], RGB(0, 255, 0));
    EXPECT_EQ(pb.getData()[2], RGB(0, 0, 255));
    EXPECT_EQ(pb.getData()[3], RGB(255, 255, 255));
}

TEST(PixelBuffer, loadImgCrop) {

    fixtures::TmpFileHelper tf(fixtures::png4x4);

    PixelBuffer pb;
    EXPECT_TRUE(pb.loadImg(tf.fn_.c_str(), Bounds{1, 0, 0, 0}));
    EXPECT_EQ(pb.width_, 3);
    EXPECT_EQ(pb.height_, 4);
    EXPECT_EQ(pb.getOffsetX(), 1);
    EXPECT_EQ(pb.getOffsetY(), 0);

    EXPECT_EQ(pb.data_[0], RGB(0, 255, 0));
    EXPECT_EQ(pb.data_[1], RGB(0, 0, 255));
    EXPECT_EQ(pb.data_[2], RGB(255, 255, 255));
}

TEST(PixelBuffer, loadImgCropLimits) {
    fixtures::TmpFileHelper tf(fixtures::png256hi); //256x256
    PixelBuffer pb;

    // Test cropLeft limit (should clamp to fw-1 = 3)
    EXPECT_TRUE(pb.loadImg(tf.fn_.c_str(), Bounds{10, 20})); //subtract 10 and 20 from width, height
    EXPECT_EQ(pb.getOffsetX(), 10);
    EXPECT_EQ(pb.width_, 246);
    EXPECT_EQ(pb.height_, 236);

    // Test cropBottom limit (should clamp to fh = 4)
    EXPECT_TRUE(pb.loadImg(tf.fn_.c_str(), Bounds{0, 0, 100, 200})); //limit to being a 100x200 cropped img
    EXPECT_EQ(pb.width_, 100);
    EXPECT_EQ(pb.height_, 200);
}
