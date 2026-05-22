#include "PixelBuffer.h"
#include "log.h"
#include <PNGdec.h>


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

static PNG _png;

#ifdef ARDUINO
#include <SD.h>
static void * png_open(const char *fn, int32_t *s) {
    File *f = new File(SD.open(fn));
    if (!f || !*f) { if (f) delete f; return nullptr; }
    *s = f->size(); return (void *)f;
}
static void png_close(void *h) { File *f = (File *)h; if (f) { f->close(); delete f; } }
static int32_t png_read(PNGFILE *h, uint8_t *b, int32_t l) { return static_cast<File *>(h->fHandle)->read(b, l); }
static int32_t png_seek(PNGFILE *h, int32_t p) { return static_cast<File *>(h->fHandle)->seek(p); }
#else
static void * png_open(const char *fn, int32_t *s) {
    FILE *f = fopen(fn, "rb"); if (!f) return nullptr;
    fseek(f, 0, SEEK_END); *s = ftell(f); fseek(f, 0, SEEK_SET); return (void *)f;
}
static void png_close(void *h) { if (h) fclose((FILE *)h); }
static int32_t png_read(PNGFILE *h, uint8_t *b, int32_t l) { return fread(b, 1, l, (FILE *)h->fHandle); }
static int32_t png_seek(PNGFILE *h, int32_t p) { return fseek((FILE *)h->fHandle, p, SEEK_SET) == 0; }
#endif

static int png_draw(PNGDRAW *p) {
    PixelBuffer *pb = (PixelBuffer *)p->pUser;
    _png.getLineAsRGB565(p, pb->getPixelPtrAbs(0, p->y), PNG_RGB565_LITTLE_ENDIAN, 0);
    return 1; //continue decoding
}

int freeHeap(); //in maprenderer.cpp

bool PixelBuffer::loadImg(const char* path) {
    if (!path || !path[0]) return false;
    if (_png.open(path, png_open, png_close, png_read, png_seek, png_draw) != PNG_SUCCESS) return false;

    MAP_LOG("pxl::load %s opened (free: %u)", path, freeHeap());
    if (!allocate(_png.getWidth(), _png.getHeight())) {
        MAP_LOG("pxl::load failed, OOM (free: %u)", freeHeap());
        _png.close(); return false;
    }

    int rc = _png.decode(this, 0);
    _png.close();
    MAP_LOG("pxl::load %s finished (free: %u)", path, freeHeap());
    return rc == PNG_SUCCESS;
}
