#include "PixelBuffer.h"
#include "log.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"


PixelBuffer::~PixelBuffer() {
    clear();
}

bool PixelBuffer::allocate(uint16_t width, uint16_t height) {
    clear();
    data_ = (uint16_t*)malloc(width * height * sizeof(pixel_t));
    if (data_) {
        width_ = width;
        height_ = height;
        return true;
    }
    return false;
}

void PixelBuffer::clear() {
    if (data_) free(data_);
    data_ = nullptr;
    width_ = height_ = 0;
}

void PixelBuffer::drawPixelAbs(coord_t x, coord_t y, pixel_t value) {
    if (data_ && (x <= width_ && y < height_)) {
        pixel_t* p = getPixelPtrAbs(x, y);
        *p = value;
    }
}

pixel_t* PixelBuffer::getPixelPtrAbs(coord_t x, coord_t y) {
    return &data_[y * width_ + x];
}

#ifdef ARDUINO
#include <SD.h>
// STB callbacks for Arduino SD library
static int stb_read(void* user, char* data, int size) {
    File* f = static_cast<File*>(user);
    return f->read((uint8_t*)data, size);
}
static void stb_skip(void* user, int n) {
    File* f = static_cast<File*>(user);
    f->seek(f->position() + n);
}
static int stb_eof(void* user) {
    File* f = static_cast<File*>(user);
    return !f->available();
}
#else
// STB callbacks for standard C++ FILE (for unit testing)
static int stb_read(void* user, char* data, int size) {
    return fread(data, 1, size, (FILE*)user);
}
static void stb_skip(void* user, int n) {
    fseek((FILE*)user, n, SEEK_CUR);
}
static int stb_eof(void* user) {
    return feof((FILE*)user);
}
#endif

bool PixelBuffer::loadImg(const char* path) {
    if ((path == nullptr) || (path[0] == 0)) return false;
    stbi_io_callbacks callbacks = {stb_read, stb_skip, stb_eof};
    void* fh = nullptr;
#ifdef ARDUINO
    File f = SD.open(path);
    if (!f) return false;
    fh = &f;
#else
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fh = f;
#endif

    int imgW, imgH, n;
    constexpr int req_comp = 3;
    unsigned char *img = stbi_load_from_callbacks(&callbacks, fh, &imgW, &imgH, &n, req_comp);
#ifdef ARDUINO
    f.close();
#else
    fclose(f);
#endif

    if (!img) {
        MAP_LOG("loadBitmap(%s) failed: %s\n", path, stbi_failure_reason());
        return false;
    }
    if (data_) {
        MAP_LOG("loadBitmap(%s) clearing existing img first\n", path);
        clear();
    }
    if (!allocate(imgW, imgH)) {
        stbi_image_free(img);
        return false;
    }

    auto dest = getPixelPtrAbs(0, 0);
    // RGB888 to RGB565
    for (uint32_t i = 0; i < (uint32_t)imgW * imgH; i++) {
        uint8_t r = img[i * req_comp + 0];
        uint8_t g = img[i * req_comp + 1];
        uint8_t b = img[i * req_comp + 2];
        dest[i] = RGB(r, g, b);
    }
    stbi_image_free(img);
    return true;
}
