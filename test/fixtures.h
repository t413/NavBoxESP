#pragma once
#include <string>
#include <vector>

std::string fmtstr(const char* fmt, ... );
std::string basename(const std::string& path);
std::string testname();

// A minimal 4x4 rainbow png. starts with r, g, b, w, then more rainbow for the rest.
extern const std::vector<uint8_t> png4x4;
extern const std::vector<uint8_t> png256hi;

struct TmpFileHelper {
    std::string fn_;
    TmpFileHelper(const std::vector<uint8_t> &img);
    ~TmpFileHelper() { rm(); }
    void rm();
};
