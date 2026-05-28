#include <gtest/gtest.h>
#include "fixtures.h"
#include <log.h>
#include <MapRenderer.h>
#include <MapLayer.h>
#include <TrackLog.h>

using namespace std;
using namespace fixtures;

constexpr string TILE_FMT_END = "/%d/%d/%d.png";
constexpr string TILE_FMT = "/tmp" + TILE_FMT_END;
constexpr int TEST_Z = 10;
constexpr int TEST_X = 512;
constexpr int TEST_Y = 512;

std::filesystem::path OSM_TILES_STASH = "/Users/timo/Documents/t4ds/static/ext/tiles"; //TODO .. download tiles here?

std::string tilePath(int z, int x, int y) {
    return fmtstr(TILE_FMT.c_str(), z, x, y);
}

TEST(MapRenderer, latlonToTile) {
    double x,y;
    MapRenderer::_latLonToTileF(37.87, -122.32, 14, x, y);
    MAP_LOG("xy %.2f, %.2f", x, y);
    EXPECT_EQ((int) x, 2625);
    EXPECT_EQ((int) y, 6327);

    MapRenderer::_latLonToTileF(37.87, -122.32, 17, x, y);
    MAP_LOG("xy %.2f, %.2f", x, y);
    EXPECT_EQ((int) x, 21000);
    EXPECT_EQ((int) y, 50618);

    // Test lat <-> tileY roundtrip
    double lat = 37.87;
    double ty = MapRenderer::_latToTileY(lat, 14);
    EXPECT_NEAR(ty, 6327, 1.0);
    EXPECT_NEAR(MapRenderer::_tileYToLat(ty, 14), lat, 0.0001);

    // Test equator
    EXPECT_NEAR(MapRenderer::_latToTileY(0.0, 10), 512.0, 0.0001);
    EXPECT_NEAR(MapRenderer::_tileYToLat(512.0, 10), 0.0, 0.0001);
}

TEST(MapRenderer, SetupAndProjection) {
    fixtures::LvglTestEnv env(240, 135);
    fixtures::TmpFileHelper file(fixtures::png256hi, tilePath(TEST_Z, TEST_X, TEST_Y));

    MapRenderer map;
    auto ret = map.begin(env.base_, env.width_, env.height_, TILE_FMT.c_str());
    EXPECT_TRUE(ret);

    map.setCenter({0.0, 0.0}, TEST_Z);
    env.draw(); //do a full lvgl render

    double tx = 0, ty = 0;
    map._latLonToTileF(map.lat(), map.lon(), map.zoom(), tx, ty);
    MAP_LOG("<0.0,0.0> -> %0.2f, %0.2f", tx, ty);
    EXPECT_NEAR(tx, TEST_X, 1.0);
    EXPECT_NEAR(ty, TEST_Y, 1.0);

    lv_coord_t px, py;
    map.project(0.0, 0.0, px, py);

    EXPECT_EQ(px, 120);
    EXPECT_EQ(py, 67);
    env.save();
 }

TEST(MapRenderer, PanLogic) {
    MapRenderer map;
    map.begin(NULL, 300, 300, TILE_FMT.c_str());
    map.setCenter({0.0, 0.0});
    lv_coord_t center_px, center_py;
    map.project(0.0, 0.0, center_px, center_py);
    EXPECT_EQ(center_px, 150);
    EXPECT_EQ(center_py, 150);

    for (uint8_t z = 2; z < 18; z++) {
        map.setCenter({0.0, 0.0});
        map.setZoom(z);
        map.panPx(100, 0); //move not entirely out of view
        EXPECT_NEAR(map.lat(), 0.0, 0.001); //unchanged

        lv_coord_t px, py;
        map.project(0.0, 0.0, px, py);
        EXPECT_TRUE(map.isVisible(px, py));

        map.panPx(0, 100); //move not entirely out of view
        map.project(0.0, 0.0, px, py);
        EXPECT_TRUE(map.isVisible(px, py));
        MAP_LOG("mv z%d -> <%5.5f,%5.5f> -> [%d,%d]", z, map.lat(), map.lon(), px, py);
        EXPECT_EQ(px, 50);
        EXPECT_EQ(py, 50);
    }
}

TEST(MapRenderer, RealMapPositionRender) {
    fixtures::LvglTestEnv env(300, 200);
    MapRenderer map;
    // map.cropmode_ = true;
    if (!filesystem::is_directory(OSM_TILES_STASH)) {
        MAP_LOG("WARNING CAN'T RUN TEST WITHOUT TILES");
        return;
    }
    string tfmt = string(OSM_TILES_STASH) + TILE_FMT_END;
    auto ret = map.begin(env.base_, env.width_, env.height_, tfmt.c_str());
    EXPECT_TRUE(ret);

    map.setCenter({37.87, -122.32}, 16); //CCP at zoom 16
    auto mkrs = map.getMarkerLayer();
    auto dotidx  = mkrs->add(Marker({37.87050,-122.32000}, 15));
    auto homeidx = mkrs->add(Marker({37.87125,-122.31767}, 14, 0x00ff00, 'H' ));
    int saveidx = 0;
    auto drawsave = [&env,&map,&saveidx](string extra="") {
        env.draw(); //do a full lvgl render
        env.save(fmtstr("_change%d-z%d-m%d", saveidx++, map.zoom(), map.magnification()) + extra);
    };
    drawsave();

    map.invalidate();
    drawsave();

    MAP_LOG("SETTING DOT");
    map.getMarkerLayer()->updatePoint(dotidx, {37.8705,-122.320});
    MAP_LOG("SET DOT DONE");
    map.invalidate();
    drawsave();

    map.setCenter({37.8705,-122.320}); //center map on dot
    drawsave();

    map.setZoom(15, 2); //zoomed out, maginified-in
    drawsave();

    map.setZoom(14, 3); //zoomed out, maginified-in
    drawsave();

    auto zoom = map.findBestZoomWithTiles(map.getCenter(), 30);
    MAP_LOG("found findBestZoomWithTiles -> %d", zoom);

    map.setZoom(20); //auto zoom!
    drawsave("autozoom");

    for (zoom_t z = 20; z > 5; z--) {
        map.setZoom(z);
        MAP_LOG("zoom z%d -> %d (%d)", z, map.zoom(), map.magnification());
    }
    // exit(1);
}

TEST(MapRenderer, TrackLayerCircle) {
    fixtures::LvglTestEnv env(300, 200);
    TrackPoint center(37.87, -122.32);

    MapRenderer map;
    string tfmt = string(OSM_TILES_STASH) + TILE_FMT_END;
    map.begin(env.base_, env.width_, env.height_, tfmt.c_str());

    TrackLog track("/tmp");
    TrackLayer tl(&map, 0xFF0000, &track);
    map.addLayer(&tl);

    auto& mkrs = *(map.getMarkerLayer());
    auto dotidx  = mkrs.add(Marker(center, 16));
    auto homeidx = mkrs.add(Marker(center, 18, 0x00ff00, 'H' ));

    track.newRecording(1936610000);
    const char* testPath = track.getRecPath();

    //make a spiral
    for (int i = 0; i <= 360*2; i += 10) {
        auto p = center.fromDistHeading(180.0 - i/5, (double)i);
        track.addPoint(p);
        if (i==20) mkrs.updatePoint(dotidx, p);
    }
    track.stopRecording();
    MAP_LOG("rec %d kept %d", track.recordedPoints_, (int) track.points().size());

    map.setCenter(center, 16); //CCP at zoom 16. also redraws layers
    env.draw();
    env.save("_trackcircle");

    map.removeLayer(&tl);
    remove(testPath);
}
