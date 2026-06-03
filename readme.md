# NavBox - Offline GPS Firmware

Portable offline GPS navigation with the $30 M5 Cardputer

[![Top Language](https://img.shields.io/github/languages/top/t413/NavBoxESP?style=flat-square)](https://github.com/t413/NavBoxESP)
[![License](https://img.shields.io/badge/License-GPL--3.0-green?style=flat-square)](https://github.com/t413/NavBoxESP/blob/main/LICENSE)
![Tests](https://img.shields.io/github/actions/workflow/status/t413/NavBoxESP/ci.yaml?style=flat-square)

<p align="center">
  <a href="https://t413.com/p/projects/NavBox/heart-small.gif"><img src="https://t413.com/p/projects/NavBox/heart-small.gif" alt="maps in action"></a>
</p>

- Ultra-lightweight offline mapping for your M5 Cardputer (more devices coming!)
- GPX track viewing, recording, and saving. Downlad hikes from AllTrails, etc, and actually navigate in the wild!
- Zoom and pan navigation with device-local tile rendering from the SD card
- Built on [NavBoxLib](https://github.com/t413/NavBoxLib), my open-source embedded mapping engine

## Installation

The easiest way to get started is through the **M5 Launcher**:

1. Visit the [Launcher webflasher](https://bmorcelli.github.io/Launcher/webflasher.html)
2. Install to your Cardputer
3. Install NavBox. Either:
   - From Launcher on your device use "OTA" area to find NavBox and download and/or install
   - Download the latest release from [Releases](https://github.com/t413/NavBoxESP/releases)
   - Get the bleeding-edge CI build from [GitHub Actions](https://github.com/t413/NavBoxESP/actions)
4. Add a GPS, see below

## Map Data

NavBox uses standard OpenStreetMap tiles (256×256 format). Download map data for anywhere on earth via:

- Download a global basemap zip from [Releases](https://github.com/t413/NavBoxESP/releases) first! This saves a lot on my server load to start with this.
- **Web tool**: Use my [AlphaMap Web Tile Downloader](https://t413.com/go/alphamaptool?ref=ghnavbox)
   * Select a folder, download tiles.
   * Copy the `osm` directory to your SD card at `/maps/osm`.
- **Command line**: Use [osm2edgetx](https://github.com/t413/osm2edgetx)
  ```bash
  cd /Volumes/SD_CARD_PATH/maps/osm/
  python osm2edgetx.py --osm . --fetch "37.87,-122.32" --radius 5 --zoom 17
  ```

## GPS Support

Any standard GPS should work. I've built and tested with/for the

- M5 GPS Unit ([M5 docs](https://docs.m5stack.com/en/unit/Unit-GPS%20v1.1)) plugged into the GROVE port on the left
- M5 Cap Lora ([M5 docs](https://docs.m5stack.com/en/cap/Cap_LoRa-1262)) plugged in on top

The firmware tries both on boot, both should work!

## Not AI

Written by me, [@t413](https://github.com/t413), a real firmware dev. Not vibecoded. My documentation isn't overly verbose bs, and this thing actually works. Each detail tested _on hardware_ and commited piece-by-piece with OG TDD principles and SWE good practices. As much as possible is tested throughly.

### A starting point for other projects

This project is a fantastic intro to using [LVGL](https://lvgl.io/) to make beautiful responsive modern UIs with minimal memory&CPU usage. The initial difficult part of this project, the mapping engine, is even broken out to my [NavBoxLib](https://github.com/t413/NavBoxLib) compodent so the app portion stays simple. If you're looking to make a firmware project this repository is a great place to start. It has:

- Working dependencies with M5 libraries. Not all versions work, it took a lot of testing and debugging to get dependencies pinned and things compiling and running.
- Working LVGL menuing, lists, overlays, etc
- Settings system with persistance to the filesystem
- GIT Version injection so the firmware itself knows what version it is
- Unit testing of views, complete with screenshots saved as PNG files for review
- CI. It runs unit tests on every push and archives all firmware files with the repository name, target name, and git version. Perfect for deployment
- Modular multi-target support (work in progress)


## Helping

If you've got suggestions, ideas, or want to help please open an Issue here on github! Code contributions with PR's are welcome, but it's always easier if you can come to some conscensious on the need and path first.

## License

This project is GPLv3. See [LICENSE](https://github.com/t413/NavBoxESP/blob/main/LICENSE) for more info!
