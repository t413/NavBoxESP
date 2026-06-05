#pragma once
#include <string>

// Writes preloaded base tiles to SD card on first boot
// Returns true if successful or already installed
bool ensureBaseTilesOnSD(const char* basedir);

bool mkdirs(const std::string& path);
