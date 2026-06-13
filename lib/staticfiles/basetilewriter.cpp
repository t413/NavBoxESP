#include "basetilewriter.h"
#include "basetiles.h"
#include <navboxlib/log.h>
#include <navboxlib/fileclass.h>
#include <string>

using namespace fs;
using namespace std;

bool mkdirs(const string& path) {
    if (SD.exists(path.c_str())) return true;

    size_t pos = 0;
    while ((pos = path.find('/', pos + 1)) != string::npos) {
        string sub = path.substr(0, pos);
        if (!sub.empty() && !SD.exists(sub.c_str())) {
            if (!SD.mkdir(sub.c_str())) return false;
        }
    }
    if (!SD.exists(path.c_str())) {
        if (!SD.mkdir(path.c_str())) return false;
    }
    return true;
}

bool ensureBaseTilesOnSD(const char* basedir) {
    if (kBaseTileCount == 0) return true;

    auto mkpath = [basedir](const StaticTile& tile) {
        return string(basedir) + "/" + to_string(tile.z) + "/" + to_string(tile.x) + "/" + to_string(tile.y) + ".png";
    };

    // Check if the last tile exists as a proxy for installation completion
    const auto& lastt = kBaseTiles[kBaseTileCount - 1];
    auto lastPath = mkpath(lastt);
    if (SD.exists(lastPath.c_str())) {
        MAP_LOG("[Tiles] Already installed");
        return true;
    }
    MAP_LOG("[Tiles] Installing %u base tiles...", kBaseTileCount);

    // Write each tile
    for (uint32_t i = 0; i < kBaseTileCount; i++) {
        const StaticTile& tile = kBaseTiles[i];
        string tpath = mkpath(tile);

        string dirPath = tpath;
        size_t lastSlash = dirPath.find_last_of('/');
        if (lastSlash != string::npos) {
            dirPath = dirPath.substr(0, lastSlash);
        }
        if (!mkdirs(dirPath)) return false;

        File f = SD.open(tpath.c_str(), FILE_WRITE); // Write tile file
        if (!f) {
            MAP_LOG("[Tiles] Failed to open %s for write", tpath.c_str());
            return false;
        }
        size_t written = f.write(tile.data, tile.length);
        f.close();
        if (written != tile.length) {
            MAP_LOG("[Tiles] Write mismatch for %s: %d vs %u", tpath.c_str(), (int)written, tile.length);
            return false;
        }
        if ((i + 1) % 5 == 0)
            MAP_LOG("[Tiles] Installed (%u/%u/%u) %u/%u", tile.z, tile.x, tile.y, i + 1, kBaseTileCount);
    }
    return true;
}
