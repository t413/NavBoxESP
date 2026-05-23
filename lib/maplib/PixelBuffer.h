#pragma once
#include <stdint.h>

typedef uint16_t coord_t;
typedef uint16_t pixel_t;
#define GET_RED(color) (((color) & 0xF800) >> 8)
#define GET_GREEN(color) (((color) & 0x07E0) >> 3)
#define GET_BLUE(color) (((color) & 0x001F) << 3)
#define RGB(r, g, b) (uint16_t)((((r) & 0xF8) << 8) + (((g) & 0xFC) << 3) + (((b) & 0xF8) >> 3))
struct Bounds;

/// PixelBuffer, generic buffer to hold image data with optional sparse mode.
class PixelBuffer {
public:
    PixelBuffer() : data_(nullptr), width_(0), height_(0), offsetX_(0), offsetY_(0) { }
    ~PixelBuffer();

    bool allocate(coord_t width, coord_t height, coord_t offsetX = 0, coord_t offsetY = 0);
    void clear();
    void drawPixelAbs(coord_t x, coord_t y, pixel_t value);
    pixel_t* getPixelPtrAbs(coord_t x, coord_t y);

    pixel_t* getData() const { return data_; }
    uint32_t size() const { return width_ * height_; }
    bool valid() const { return width_ + height_ > 0 && data_ != nullptr; }

    /// Load PNG image with optional cropping.
    /// left/right crop is saved as offsetN_, right/bttm values become the new width/heights, 0 crops none
    bool loadImg(const char* path, const Bounds &);

    coord_t getOffsetX() const { return offsetX_; }
    coord_t getOffsetY() const { return offsetY_; }
    bool isSparse() const { return offsetX_ != 0 || offsetY_ != 0; }

    pixel_t* data_; // RGB565 data for LVGL compatibility
    coord_t width_, height_;

private:
    coord_t offsetX_, offsetY_;  // Offset within original image for sparse buffers
};

struct Bounds {
    coord_t left = 0, top = 0, right = 0, bttm = 0;
    operator bool() const { return left || top || right || bttm; }
};
