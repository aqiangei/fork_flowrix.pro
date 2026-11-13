#include "test.h"
#include "test_data.h"
#include "server/replication.h"
#include "server/systems/combat.h"
#include "server/systems/loot.h"
#include "shared/core/world.h"
#include "shared/game/components.h"

using namespace flix;

namespace {
struct RubyArena {
    World world;
    CombatSystem combat;
    Entity player = world.create();
    Entity mob = world.create();
    RubyArena() {
        world.add<PlayerTag>(player);
        world.add<Faction>(player, Faction{Team::Players, false});
        world.add<PlayerModifiers>(player);
        world.add<MobTag>(mob);
        world.add<Faction>(mob, Faction{Team::Hostiles, false});
        world.add<Health>(mob, Health{100000, 100000, 0, 0});
    }
    DamageResult hit(DamageKind kind = DamageKind::Lightning, double damage = 10) {
        return combat.applyDamage(world, mob, player, damage, 1000, kind);
    }
};

TEST(ruby_repeats_are_bounded_and_stop_when_the_mob_dies) {
    RubyArena arena;
    auto& mods = arena.world.get<PlayerModifiers>(arena.player);
    mods.lightningRepeatChance = 1;
    mods.maxLightningRepeats = 4;
    CHECK_NEAR(arena.hit().applied, 50, 1e-9);
    arena.world.get<Health>(arena.mob).current = 25;
    const DamageResult lethal = arena.hit();
    CHECK_NEAR(lethal.applied, 25, 1e-9);
    CHECK(lethal.killed);
    CHECK(arena.world.has<Dead>(arena.mob));
    CHECK(arena.hit().refused);
}

TEST(ruby_only_repeats_lightning_from_its_wearer_against_hostile_mobs) {
    RubyArena arena;
    CHECK_NEAR(arena.hit().applied, 10, 1e-9);
    auto& mods = arena.world.get<PlayerModifiers>(arena.player);
    mods.lightningRepeatChance = 1;
    mods.maxLightningRepeats = 4;
    CHECK_NEAR(arena.hit(DamageKind::Direct).applied, 10, 1e-9);
    CHECK_NEAR(arena.hit(DamageKind::Poison).applied, 10, 1e-9);
    // Real petal lightning reaches combat through an owned damage field.
    const Entity field = arena.world.create();
    arena.world.add<GroundEffect>(field, GroundEffect{GroundEffectKind::Poison, arena.player});
    CHECK_NEAR(arena.combat.applyDamage(arena.world, arena.mob, field, 10, 1000,
                                      DamageKind::Lightning).applied, 50, 1e-9);
    const Entity other = arena.world.create();
    arena.world.add<PlayerTag>(other);
    arena.world.add<Faction>(other, Faction{Team::Players, false});
    CHECK_NEAR(arena.combat.applyDamage(arena.world, arena.mob, other, 10, 1000,
                                      DamageKind::Lightning).applied, 10, 1e-9);
    arena.world.get<Health>(arena.mob).invulnerableUntilMillis = 5000;
    CHECK(arena.hit().refused);
}

TEST(ruby_common_repeat_probability_is_high_but_not_guaranteed) {
    RubyArena arena;
    auto& mods = arena.world.get<PlayerModifiers>(arena.player);
    mods.lightningRepeatChance = 0.8;
    mods.maxLightningRepeats = 1;
    int repeats = 0;
    for (int i = 0; i < 1000; ++i) {
        const double applied = arena.hit(DamageKind::Lightning, 1).applied;
        CHECK(applied == 1 || applied == 2);
        repeats += applied == 2;
    }
    CHECK(repeats > 730);
    CHECK(repeats < 870);
}

TEST(ruby_repeats_lightning_on_the_training_dummy) {
    RubyArena arena;
    arena.world.remove<MobTag>(arena.mob);
    arena.world.add<NpcTag>(arena.mob);
    arena.world.add<Npc>(arena.mob).cutterPassable = true;
    arena.world.add<NetId>(arena.mob, NetId{7});
    auto& mods = arena.world.get<PlayerModifiers>(arena.player);
    mods.lightningRepeatChance = 1;
    mods.maxLightningRepeats = 4;
    EventQueue events;
    arena.combat.beginTick(arena.world, 1000, 1.0 / 30.0, events);
    CHECK_NEAR(arena.hit().applied, 50, 1e-9);
    CHECK_EQ(events.events().size(), std::size_t(5));
    for (const WireEvent& event : events.events()) {
        CHECK(event.kind == net::EventKind::Damage);
        CHECK_EQ(event.dealtBy, arena.player);
        CHECK_NEAR(event.amount, 10, 1e-9);
    }
    arena.world.get<Npc>(arena.mob).cutterPassable = false;
    CHECK_NEAR(arena.hit().applied, 10, 1e-9);
}

TEST(ruby_rarity_stats_and_fixed_and_random_drops_are_available) {
    const ContentRegistry& registry = testsupport::shippedContent();
    const auto ruby = registry.petalIndex("ruby");
    CHECK(ruby != kInvalidIndex);
    if (ruby == kInvalidIndex) return;
    double previous = 0;
    for (int tier = 0; tier < kLadderRarityCount; ++tier) {
        const auto& stats = registry.petalStats(ruby, static_cast<Rarity>(tier));
        CHECK(stats.damage > 0);
        CHECK(stats.health > 0);
        CHECK(stats.lightningRepeatChance >= previous);
        CHECK(stats.lightningRepeatChance <= 0.95);
        CHECK_EQ(stats.maxLightningRepeats, tier == rarityIndex(Rarity::Apex) ? 7 : 2 + tier / 3);
        previous = stats.lightningRepeatChance;
    }
    CHECK_NEAR(registry.petalStats(ruby, Rarity::Common).lightningRepeatChance, 0.8, 1e-9);
    CHECK_NEAR(registry.petalStats(ruby, Rarity::Apex).lightningRepeatChance, 0.95, 1e-9);
    CHECK_NEAR(registry.petalStats(ruby, Rarity::Universal).lightningRepeatChance, 0.95, 1e-9);
    CHECK_EQ(registry.petalStats(ruby, Rarity::Universal).maxLightningRepeats, 7);
    CHECK(!registry.petal(ruby).image.empty());
    DropTables drops;
    std::string error;
    CHECK(drops.load(registry, testsupport::dataFile("mob_drops.json"), error));
    for (const char* mob : {"garbage", "server"}) {
        bool found = false;
        for (const auto& row : drops.forMob(registry.mobIndex(mob))) {
            found = found || row.petalIndex == ruby;
        }
        CHECK(found);
    }
    bool random = false;
    for (auto index : drops.droppablePetals()) random = random || index == ruby;
    CHECK(random);
}
TEST(apex_ruby_allows_seven_extra_hits_and_clamps_the_combat_limit) {
    RubyArena arena;
    auto& mods = arena.world.get<PlayerModifiers>(arena.player);
    mods.lightningRepeatChance = 1;
    mods.maxLightningRepeats = 9;
    CHECK_NEAR(arena.hit().applied, 80, 1e-9);
}

TEST(cutter_has_two_drop_sources) {
    const auto& registry = testsupport::shippedContent();
    const auto cutter = registry.petalIndex("cutter");
    CHECK(cutter != kInvalidIndex);
    DropTables drops;
    std::string error;
    CHECK(drops.load(registry, testsupport::dataFile("mob_drops.json"), error));
    for (const char* mob : {"server", "digger"}) {
        bool found = false;
        for (const auto& row : drops.forMob(registry.mobIndex(mob)))
            found = found || row.petalIndex == cutter;
        CHECK(found);
    }
}
} // namespace
