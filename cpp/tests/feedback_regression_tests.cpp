#include "test.h"
#include "client/ui/content_i18n.h"
#include "client/ui/i18n.h"
#include "server/splitter_placement.h"
#include "server/respawn_placement.h"

using namespace flix;

TEST(feedback_respawn_does_not_cross_box_walls) {
    Terrain terrain;
    for (int y = 0; y < terrain.tileRows(); ++y) terrain.setTile(10, y, Tile::Solid);
    for (double radius : {24.0, 48.0, 100.0}) {
        for (bool left : {false, true}) {
            const double edge = (left ? 10.0 : 11.0) * kTileSize;
            const Vec2 corpse{edge + (left ? -radius - 1 : radius + 1), 20.5 * kTileSize};
            const Vec2 p = respawnPosition(terrain, Realm::Overworld, corpse, radius);
            CHECK_NEAR(distanceSq(p, corpse), 0.0, 1e-9);
            const Vec2 tight{edge + (left ? -1.0 : 1.0), corpse.y};
            const Vec2 cleared = respawnPosition(terrain, Realm::Overworld, tight, radius);
            CHECK(!terrain.blocked(cleared, Realm::Overworld));
            CHECK(left ? cleared.x <= edge - radius + 0.001 : cleared.x >= edge + radius - 0.001);
        }
    }
}

TEST(feedback_search_accepts_both_languages_in_either_ui_language) {
    const auto previous = ui::language();
    for (auto lang : {ui::Language::En, ui::Language::Zh}) {
        ui::setLanguage(lang);
        CHECK(ui::petalMatchesSearch("ruby", "Ruby", "红宝石"));
        CHECK(ui::petalMatchesSearch("ruby", "Ruby", " RUBY "));
        CHECK(ui::petalMatchesSearch("pyramid_map", "Pyramid Map", "金字塔"));
        CHECK(ui::petalMatchesSearch("pyramid_map", "Pyramid Map", "pyramid_map"));
        CHECK(!ui::petalMatchesSearch("ruby", "Ruby", "叶子"));
        CHECK(ui::petalMatchesSearch("ruby", "Ruby", " \t "));
        CHECK(ui::petalMatchesSearch("future_petal", "Future Petal", "Future"));
    }
    ui::setLanguage(previous);
}

TEST(feedback_search_gallery_matches_chinese_english_and_ids_in_both_languages) {
    const auto previous = ui::language();
    for (auto lang : {ui::Language::En, ui::Language::Zh}) {
        ui::setLanguage(lang);
        CHECK(ui::mobMatchesSearch("server", "Server", "服务器"));
        CHECK(ui::mobMatchesSearch("server", "Server", " SERVER "));
        CHECK(ui::mobMatchesSearch("jellyfish", "Jellyfish", "水母"));
        CHECK(ui::mobMatchesSearch("jellyfish", "Jellyfish", "JELLY"));
        CHECK(ui::mobMatchesSearch("baby_ant", "Baby Ant", "baby_ant"));
        CHECK(ui::mobMatchesSearch("future_mob", "Future Mob", "future"));
        CHECK(ui::mobMatchesSearch("server", "Server", "\t "));
        CHECK(!ui::mobMatchesSearch("server", "Server", "水母"));
        CHECK(!ui::mobMatchesSearch("jellyfish", "Jellyfish", "not-a-mob"));
    }
    ui::setLanguage(previous);
}

TEST(feedback_splitter_halves_keep_full_body_clearance_near_either_wall_face) {
    Terrain terrain;
    for (int y = 0; y < terrain.tileRows(); ++y) terrain.setTile(10, y, Tile::Solid);
    const double west = 10.0 * kTileSize, east = 11.0 * kTileSize;
    for (double radius : {24.0, 48.0, 100.0}) {
        for (bool left : {false, true}) {
            Vec2 centre{left ? west - radius - 1.0 : east + radius + 1.0, 20.5 * kTileSize};
            auto halves = splitterPositions(terrain, Realm::Overworld, centre, radius);
            for (Vec2 half : halves) {
                CHECK(!terrain.blocked(half, Realm::Overworld));
                CHECK_NEAR(distanceSq(half, terrain.resolveCircle(half, radius, Realm::Overworld)), 0, 0.001);
                CHECK(left ? half.x <= west - radius + 0.001 : half.x >= east + radius - 0.001);
                // Repeated contact shoves from a mob must not undo the safe split.
                for (int tick = 0; tick < 20; ++tick) {
                    stepCollide(terrain, Realm::Overworld, half, Vec2{left ? 200.0 : -200.0, 0},
                                radius, 1.0, true, true);
                    CHECK(!terrain.blocked(half, Realm::Overworld));
                    CHECK(left ? half.x <= west - radius + 0.001 : half.x >= east + radius - 0.001);
                }
            }
        }
    }
}

TEST(feedback_splitter_still_separates_on_open_ground) {
    Terrain terrain;
    const Vec2 centre{20.5 * kTileSize, 20.5 * kTileSize};
    const auto halves = splitterPositions(terrain, Realm::Overworld, centre, 32.0);
    CHECK_NEAR(halves[0].x, centre.x - 32.0, 0.001);
    CHECK_NEAR(halves[1].x, centre.x + 32.0, 0.001);
}

TEST(feedback_search_talents_matches_both_languages_and_distinct_tier_names) {
    const auto previous = ui::language();
    for (auto lang : {ui::Language::En, ui::Language::Zh}) {
        ui::setLanguage(lang);
        CHECK(ui::skillMatchesSearch(SkillId::PlayerHealth, 0, "花朵生命"));
        CHECK(ui::skillMatchesSearch(SkillId::PlayerHealth, 0, "  FLOWER health "));
        CHECK(ui::skillMatchesSearch(SkillId::Duplicator, 1, "三倍复制"));
        CHECK(ui::skillMatchesSearch(SkillId::Duplicator, 1, "TRIPLICATOR"));
        CHECK(!ui::skillMatchesSearch(SkillId::Duplicator, 0, "triplicator"));
        CHECK(!ui::skillMatchesSearch(SkillId::Damage, 0, "磁力"));
    }
    ui::setLanguage(previous);
}
