#pragma once
#include <cstddef>
#include <cstdint>

struct StaticTile {
    uint16_t z, x, y;
    const uint8_t* data;
    uint32_t length;
};

extern const StaticTile kBaseTiles[];
extern const uint32_t kBaseTileCount;
