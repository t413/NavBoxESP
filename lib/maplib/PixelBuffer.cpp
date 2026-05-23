#include "PixelBuffer.h"
#include "log.h"
#include <PNGdec.h>
#include <cstring>
#include <algorithm>


PixelBuffer::~PixelBuffer() {
    clear();
}

bool PixelBuffer::allocate(coord_t width, coord_t height, coord_t offsetX, coord_t offsetY) {
    clear();
    data_ = (uint16_t*)malloc(width * height * sizeof(pixel_t));
    if (data_) {
        width_ = width;
        height_ = height;
        offsetX_ = offsetX;
        offsetY_ = offsetY;
        return true;
    }
    return false;
}

void PixelBuffer::clear() {
    if (data_) free(data_);
    data_ = nullptr;
    width_ = height_ = 0;
    offsetX_ = offsetY_ = 0;
}

void PixelBuffer::drawPixelAbs(coord_t x, coord_t y, pixel_t value) {
    if (data_ && (x < width_ && y < height_)) {
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

// Context for sparse PNG decoding
struct SparsePngContext {
    PixelBuffer* buffer;
    Bounds b;
};

static int png_draw_sparse(PNGDRAW *p) {
    auto ctx = (SparsePngContext *)p->pUser;

    // Skip rows outside crop region
    if (ctx->b && (p->y < ctx->b.top || p->y >= ctx->b.bttm)) {
        return 1; // Continue decoding but skip this row
    }

    // Decode the full line into a temporary buffer
    uint16_t *lineBuffer = new uint16_t[_png.getWidth()];
    _png.getLineAsRGB565(p, lineBuffer, PNG_RGB565_LITTLE_ENDIAN, 0);

    // Copy only the cropped portion
    coord_t srcStart = ctx->b ? ctx->b.left : 0;
    coord_t srcEnd = ctx->b ? ctx->b.right : _png.getWidth();
    coord_t copyWidth = srcEnd - srcStart;
    coord_t dstY = ctx->b ? (p->y - ctx->b.top) : p->y;

    // Copy the cropped line into the buffer
    if (dstY < ctx->buffer->height_) {
        std::memcpy(
            ctx->buffer->getPixelPtrAbs(0, dstY),
            &lineBuffer[srcStart],
            copyWidth * sizeof(uint16_t)
        );
    }

    delete[] lineBuffer;
    return 1; // Continue decoding
}

int freeHeap(); // in maprenderer.cpp

bool PixelBuffer::loadImg(const char* path, const Bounds &b) {
    if (!path || !path[0]) return false;

    SparsePngContext ctx = {this, b};

    if (_png.open(path, png_open, png_close, png_read, png_seek, png_draw_sparse) != PNG_SUCCESS) return false;

    MAP_LOG("pxl::load %s opened (free: %u)", path, freeHeap());

    coord_t fw = _png.getWidth(), fh = _png.getHeight();
    coord_t aw = fw, ah = fh;

    if (ctx.b) {
        ctx.b.left = std::min(ctx.b.left, (coord_t)(fw - 1));
        ctx.b.top = std::min(ctx.b.top, (coord_t)(fh - 1));
        ctx.b.right = ctx.b.right ? std::min(ctx.b.right, fw) : fw;
        ctx.b.bttm = ctx.b.bttm ? std::min(ctx.b.bttm, fh) : fh;
        aw = ctx.b.right - ctx.b.left;
        ah = ctx.b.bttm - ctx.b.top;
        MAP_LOG("pxl::load %s [%dx%d] cropped (%d,%d,%d,%d)]", path, fw, fh, ctx.b.left, ctx.b.top, ctx.b.right, ctx.b.bttm);
    }

    if (!allocate(aw, ah, ctx.b.left, ctx.b.top)) {
        MAP_LOG("pxl::load failed, OOM (free: %d)", freeHeap());
        _png.close();
        return false;
    }

    int rc = _png.decode(&ctx, 0);
    _png.close();

    if (rc != PNG_SUCCESS) {
        clear();
        MAP_LOG("pxl::load %s decode failed", path);
        return false;
    }

    MAP_LOG("pxl::load %s finished (free: %d) [%dx%d] offset: (%d,%d)", path, freeHeap(), width_, height_, offsetX_, offsetY_);
    return true;
}
