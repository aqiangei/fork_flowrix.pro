// The changelog overlay: what changed, newest release first.
//
// An overlay rather than one of the tall list panels -- it hangs under the top
// icon row at a fixed 600x500, which is why its bounds come from
// `ChangelogPanel::bounds` and not from the shared list anchor.
//
// The table below began as the browser build's CHANGELOG array
// (src/changelog.ts, in git history; it ends at August 26, 2026), taken
// verbatim and in its own order -- oldest first -- and is walked backwards to
// paint. Every release since exists only here. A new one is appended at the
// end, with nothing to re-sort and no way to get the order wrong.
//
// Row pitch is written as literals rather than derived from the face's line
// height: the reference advances 25px under a date and 24px under a bullet
// whatever the face's own leading is, and deriving it would put every row of a
// 7896px list a little further out of place than the row above it.

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "client/ui/menu_theme.h"
#include "client/ui/menus.h"
#include "client/ui/text.h"

namespace flix {

using namespace flix::ui;

namespace {

constexpr double kPadding = 20.0;
/// Chrome above the scrolling body: the title row and the header buttons.
constexpr double kHeaderHeight = 40.0;
constexpr double kScrollbarWidth = 10.0;
/// A date row advances 25, a bullet 24, and 15 separates two entries.
constexpr double kDatePitch = 25.0;
constexpr double kBulletPitch = 24.0;
constexpr double kEntryGap = 15.0;
constexpr double kChangeSize = 14.0;
/// Bullet glyph to change text.
constexpr double kBulletIndent = 20.0;
/// One wheel notch is ~100px of deltaY in a browser, and this panel consumes
/// that unscaled, so the same gesture covers the ground it covers in the
/// reference.
constexpr double kWheelStep = 100.0;

/// The card's own colours come from kChangelogSkin; this is the border shade
/// reused as a rule and a scrollbar thumb on the body.
constexpr std::uint32_t kBodyBorder = kChangelogSkin.border;
constexpr std::uint32_t kLinkFill = 0xD8F7FFu;

constexpr int kMaxChanges = 32;

struct ChangelogEntry {
    const char* date;
    const char* changes[kMaxChanges];
};

constexpr ChangelogEntry kChangelog[] = {
    {"October 18, 2025",
     {"Added changelog"}},
    {"October 19, 2025",
     {"Added 3 new biomes",
      "New admin petal: Sparkle",
      "New admin command: spawn_special_mobs"}},
    {"October 22, 2025",
     {"New petal: Lightning",
      "New petal: Iris",
      "It is discovered that mobs have round hitboxes, so the setting is now more accurate"}},
    {"October 24, 2025",
     {"Overhauled the settings menu",
      "Ultra+ petals now have particle effects"}},
    {"November 20, 2025",
     {"New biome: Jungle"}},
    {"November 21, 2025",
     {"Some recolors and tweaks to the graphics",
      "New petal: Jelly",
      "fixes"}},
    {"November 22, 2025",
     {"New petal: Yucca",
      "New petal: Leaf",
      "Target dummy now shows DPS"}},
    {"November 23, 2025",
     {"Added skills system"}},
    {"November 29, 2025",
     {"New mob: Item Spawner",
      "New petal: Cutter",
      "New petal: Lightning Cutter",
      "New petal: Wing",
      "New biome: Sewers"}},
    {"December 3, 2025",
     {"Some UI changes",
      "Recolored some biomes",
      "New petal: Square",
      "A global notification is now shown when a rare petal is crafted"}},
    {"December 10, 2025",
     {"New petal: Blood Leaf, it damages the player when exploding",
      "Healing and self-damage now scales by rarity",
      "Cactus and Poison Cactus now give max health to the player"}},
    {"December 11, 2025",
     {"New petal: Ant Egg",
      "New petal: Fire Ant Egg",
      "Some rebalancing of mobs"}},
    {"December 15, 2025",
     {"All mobs now have eggs",
      "Added mob gallery",
      "Some optimizations"}},
    {"December 18, 2025",
     {"New ant hell map"}},
    {"December 20, 2025",
     {"Added shop, note that you cannot buy petals with real money, only stars",
      "Stars are awarded for killing mobs or claiming codes",
      "New admin command: generate_code"}},
    {"December 23, 2025",
     {"Added notifications system, check it to see when there might be an event",
      "Fixed a negative petal health bug",
      "New admin command: clear_notifications",
      "New admin command: notification"}},
    {"December 26, 2025",
     {"Changed title screen UI",
      "New admin command: give"}},
    {"December 31, 2025",
     {"New petal: Splitter",
      "New petal: Gas",
      "New petal: Bulb",
      "Petals now have physics and are attracted to mobs"}},
    {"January 19, 2026",
     {"Changed the map of most biomes",
      "Some biomes are temporarily disabled for now"}},
    {"January 22, 2026",
     {"Ocean is back!",
      "New petal: Starfish",
      "New petal: Sponge",
      "New mob: Starfish",
      "New mob: Sponge"}},
    {"January 31, 2026",
     {"New biome: MATRIX",
      "˜´∑ µø∫Ú øßå",
      "˜ªº˜˚¬ˆçß••",
      "New petal: ∆å√åßç®ˆπ†"}},
    {"February 16, 2026",
     {"New petal: Glass",
      "New petal: Third Eye",
      "New petal: Corn",
      "Commands are now suggested in the chat"}},
    {"February 20, 2026",
     {"Buffed droprates of ultras by 20x",
      "Changed the UI of mobs"}},
    {"February 21, 2026",
     {"Bugfix"}},
    {"February 22, 2026",
     {"Fixed an animation loop bug"}},
    {"February 22-March 25, 2026",
     {"Fixed performance issues",
      "Your settings now save",
      "Fixed teleporter system",
      "Backend changes",
      "FPS should be higher now, and ping should be lower"}},
    {"March 27, 2026",
     {"New petal: Faster",
      "More bug fixes and UI improvements"}},
    {"April 2, 2026",
     {"New mob: Sun",
      "New petal: Pollen",
      "Bulb now emits light",
      "Nerfed blood leaf",
      "Fixed some bugs"}},
    {"April 5, 2026",
     {"You now have a second row of loadout slots"}},
    {"April 12, 2026",
     {"Changed the UI of the skills panel",
      "Changed the UI of the inventory",
      "Changed the UI of the crafting panel",
      "Added second chance skill",
      "Fixed players getting damaged during invulnerability"}},
    {"April 13, 2026",
     {"Added apex rarity",
      "Recolored unique and apex petals"}},
    {"April 17, 2026",
     {"New petal: Powder",
      "New petal: Peas",
      "New mob: Desert Centipede",
      "New mob: Centipede"}},
    {"April 18, 2026",
     {"Faster now stacks additively instead of multiplicatively",
      "Fixed players glitching into walls when going very fast",
      "Added squads",
      "Added guilds",
      "New mob: Worker Ant",
      "New mob: Baby Ant",
      "New mob: Worker Fire Ant",
      "New mob: Baby Fire Ant",
      "A new portal has opened in desert",
      "Ant hell background looks much nicer now"}},
    {"April 19, 2026",
     {"Sun no longer drops eggs (to fix an exploit)",
      "Sun now drops glass, rock, sand, pollen, speed boost, and shield",
      "Added PVP mode",
      "Added Daily Streak",
      "Any obtained sun egg is now deleted"}},
    {"April 22, 2026",
     {"New mob: Ant Hole",
      "New mob: Fire Ant Hole"}},
    {"April 23, 2026",
     {"New petal: Magnet",
      "New petal: Air",
      "New petal: Soil",
      "Changed Ant Hole and Fire Ant Hole drops"}},
    {"April 24, 2026",
     {"Optimize the game",
      "Added API keys(you can now create discord bot)",
      "Secured the server",
      "Roach now looks better",
      "Added ultra zones"}},
    {"April 26, 2026",
     {"Changed keybinds for using items from 1-10 to U+1-10",
      "New petal: Yin Yang",
      "New petal: Lentil",
      "New petal: Bubble",
      "Clover now gives luck, and increases the rarity of mobs around you",
      "Changed petal attraction system",
      "Special petals no longer work in inactive loadout slots"}},
    {"April 30, 2026",
     {"Server now restarts every 24 hours",
      "Optimizations and bug fixes",
      "The keybinds for using items can now be swapped between U+1-10 and 1-10 in the settings menu",
      "New setting: Show Admin Commands",
      "Fixed show hitboxes not working",
      "Fixed moth rendering bug",
      "Ant Hell can now spawn bosses",
      "Added a bridge to the Mythic zone in Garden",
      "Changed sewers wall textures"}},
    {"May 3, 2026",
     {"New petal: Antennae",
      "New petal: Observer"}},
    {"May 4, 2026",
     {"Pollen drops on the ground now",
      "Changed mob spawning algorithm",
      "Bee now drops pollen"}},
    {"May 20, 2026",
     {"New petal: Bomb",
      "New petal: Flower",
      "New petal: Raindrop",
      "Optimize game"}},
    {"June 2, 2026",
     {"Added discord link"}},
    {"June 19, 2026",
     {"Server crashes should now be fixed(hopefully)",
      "Admin names are no longer shown on leaderboard",
      "Added skins menu"}},
    {"June 30, 2026",
     {"Fixed unobtainable petals being in the shop",
      "Bugfix(from discord/youtube bug reports)",
      "New link: link:https://flowrix.sussybite.dev"}},
    {"July 7, 2026",
     {"Patched exploits to get sun egg",
      "Changed bee AI",
      "Changed most mob speeds",
      "Clover now increases craft chance",
      "Added debug menu",
      "Added maze mode(testing phase, if you find exploits, please bug report in discord)",
      "Petals collected in maze increase in rarity by 1 outside of maze",
      "Petals collected outside of maze decrease by 1 in maze",
      "Only Mythic- petals are allowed in maze",
      "Maze changes each day",
      "Added absorbing(only for petals collected in maze)"}},
    {"July 8, 2026",
     {"Maze now has a seperate leveling system than the main game",
      "Made maze larger",
      "Fixed some server bugs",
      "Added mobile support",
      "Added absorb talents"}},
    {"July 9, 2026",
     {"New desert background",
      "Server and client optimizations"}},
    {"July 14, 2026",
     {"Fix client FPS",
      "New setting: GPU Acceleration",
      "Patched some glitches where players could get into walls",
      "Admin give and spawn mob commands can now have amounts",
      "New admin command: killall"}},
    {"July 20, 2026",
     {"Reworked Rose",
      "New petal: Dahlia",
      "New petal: Azalea",
      "Sandstorm now spins faster",
      "Fixed mobile login",
      "Fixed common mobs dropping better petals than uncommon mobs"}},
    {"July 26, 2026",
     {"Fixed petals reloading instantly(there are no bugs)",
      "Fixed inventory getting corrupted"}},
    {"July 29, 2026",
     {"New mob: Evil Centipede",
      "New mob: Queen Ant",
      "New mob: Digger",
      "New petal: Shell",
      "New petal: Uranium",
      "New petal: Pincer",
      "New petal: Web",
      "New petal: Guided Missile",
      "New petal: Blue Iris",
      "New petal: Stick",
      "New petal: Moon",
      "New petal: Lotus",
      "New petal: Heaviest",
      "New petal: Rice",
      "Poison damage numbers are purple now",
      "Added stalling mechanics to the game",
      "Honey slows, not a lure petal"}},
    {"August 1, 2026",
     {"Target dummies now spawn less often at garden spawn",
      "Pets and target dummies no longer have bossbars"}},
    {"August 2, 2026",
     {"Made pets smaller",
      "Nerf digger egg"}},
    {"August 3, 2026",
     {"Patched duping exploit"}},
    {"August 4, 2026",
     {"Removed some petals that were duped"}},
    {"August 6, 2026",
     {"Patched a cheat",
      "Fixed apex mobs despawning",
      "Admins can now post images in chat",
      "/spawn command now announces boss mobs"}},
    {"August 7, 2026",
     {"All players can post images in chat now(use <img src=\"Image Link\">)",
      "Fix crafting bugs",
      "Added curves to skin editor"}},
    {"August 10, 2026",
     {"New mob: Glitch Flower",
      "Reworked flower petal",
      "Added corruption",
      "New admin command: /corrupt"}},
    {"August 17, 2026",
     {"Apex mobs now have 100x HP and XP",
      "Server optimizations"}},
    {"August 23, 2026",
     {"Fix bugs",
      "Magnet no longer instantly attracts petals without playing animation",
      "Jungle coming soon",
      "Looting requirements changed"}},
    {"August 24, 2026",
     {"Balanced all mob HP/damage",
      "Changed some mob sizes"}},
    {"August 26, 2026",
     {"Fixed server crash bug"}},
    {"September 20, 2026",
      {"New map for many biomes",
      "Jellyfish now spawns lightning",
      "New mob: Leech",
      "New mob: Crab",
      "New mob: Leafbug",
      "New mob: Firefly",
      "New mob: Bush",
      "New mob: Mantis",
      "New mob: Golden Leafbug",
      "New mob: Magic Firefly",
      "Added ant hell ultra zone",
      "Fixes skins not working",
      "Changed loadout style",
      "Changed petal style",
      "Changed petal rarity colors",
      "Nerfed apex mob HP",
      "You can now change your password",
      "Slightly change hornet color and AI",
      "Dandelion now shoots projectiles",
      "Added Grant Admin button in singleplayer mode",
      "Squad menu changes",
      "New petal: Root",
      "New petal: Battery",
      "New petal: Bur",
      "Added Reload talents",
      "Sponge now shows damage",
      "Added chat bubble",
      "Changed Rock visual"
    }},
    {"September 21, 2026",
     {"Added magic petals",
      "Splitter now splits you the moment it is equipped, and clicking the "
      "loaded petal swaps you to your other half",
      "Fixed some bugs",
    }},
    {"September 25, 2026",
     {
      "Sewers overhaul",
      "New petal: Fang",
      "New petal: Pearl",
      "New petal: Claw",
      "New petal: Poo",
      "New petal: Talisman",
      "Mob aggro range bugs fixed",
      "Patched some pet duping glitches",
      "Reload talent now stops at unique; an apex tier is refunded",
      "Only a centipede's or leech's head shows a bossbar or a kill message",
    }},
    {"September 26, 2026",
     {
      "Added Pet HP talents",
      "Fixed Petal HP talent applying only 190%",
      "Pet health and damage now scale with rarity like petals, not like mobs",
      "Pet HP bars now say \"summon\"",
      "Looting a mob now takes at least 1% of its health",
      "New petal: Oranges",
      "New petal: Grapes",
      "New magic petal: Blueberries",
      "Fixed peas and grapes only shooting when attacking",
      "New petal: Salt",
      "Added oracle"
    }},
    {"September 29, 2026",
      {
      "New petal: Wax",
      "New mob: Bumble Bee",
      "Bugfixes and optimizations"
    }},
    {"October 3, 2026",
      {
      "Added Universal rarity petals",
      "Added Titan NPC",
      "Changed the size of every single mob in the game",
      "Made Computer mobs smaller",
      "Changed chat style",
      "Added different chat channels",
      "Fire ant hole now pops instead of acting like regular ant hole",
      "Queen ants always spawn from ant hole now"
      }
    },
    {"October 4, 2026",
      {
      "Added Duplicator and Triplicator talents",
      "Make moon work like florr",
      "Added Magnetism talent after Legendary Absorption"
      }
    },
    {"October 7, 2026",
      {
      "Added termites",
      "New petal: Rubber",
      "New petal: Plank",
      "New petal: Relic",
      "New petal: Tomato",
      "New petal: Compass",
      "New petal: Carrot"
      }
    },
    {"October 10, 2026",
      {
        "Added debug console for certain devices",
        "DO NOT USE DEBUG CONSOLE FOR CHEATING OR YOU WILL GET BANNED",
        "New petal: Sacrifice",
        "All consumable items are converted to petals",
        "Fixed target dummies being too large",
        "Fixed pets being too large"
      }
    },
    {"October 11, 2026",
      {
        "Added termite ant hell",
        "Completely reworked pets",
        "Added screen shake",
        "Added damage flash"
      }
    },
    {"October 13, 2026",
      {
        "New petal: Sawblade",
        "New petal: Mimic",
        "New petal: Ram",
        "New petal: Cursor",
        "New petal: Computer Chip",
        "New petal: Champion's Crown",
        "Added new drops for Computer mobs"
      }
    },
    {"October 14, 2026",
      {
        "Added Assembler",
        "Added C++ mob and petal"
      }
    },
    {"October 16, 2026",
      {
        "Added super zones in Prestige biomes",
        "Added Prestige talent",
        "Added Prestiged biomes"
      }
    },
    {"October 18, 2026",
      {
        "New mob: Ant Egg",
        "New mob: Fire Ant Egg",
        "New petal: Amulet"
      }
    },
    {"October 22, 2026",
      {
        "Added pyramid",
        "Rate limited and added moderation to chat",
        "New petal: Bandage",
        "Bugfixes and polished some UI"
      }
    },
    {"October 25, 2026",
      {
        "Changed petal physics",
        "New petal: Basil",
      }
    },
    {"October 28, 2026",
      {
        "New petal: Spaceship",
        "New petal: Cube",
        "New petal: Pentagon",
        "Improved rates of many zones",
        "Clover from soldier ant rate increased from 0.1 to 0.4",
        "Optimized server performance",
        "Fixed ocean mob AI",
        "Fixed most top UI not sliding in",
        "Admins now can level up infinitely, past level 500",
        "Added admin tiers",
        "Reworked bubble petal"
      }
    },
    {"October 30, 2026",
     {"New petal: Icosahedron",
      "Buffed Icosahedron with armor that scales with rarity"}},
    {"2026年11月3日",
     {"新增红宝石（Ruby）花瓣，补齐切面图标与各稀有度说明",
      "重复电击概率随稀有度由70%提升至95%",
      "每次放电最多追加1～4次，同一角色多颗红宝石取最强效果",
      "垃圾、服务器怪可掉落Ruby，随机花瓣池也可获得",
      "留言板优先使用当前角色昵称，未进场时使用登录名",
      "留言入口对齐AshFlor图标、下方文字、字号及间距",
      "同步移动端操作、聊天布局与中文提示修正"}},
    {"2026年11月3日（水母区）",
     {"海洋Jellyfish区常规刷怪等级提升至S（Super）",
      "少量ET（Eternal）沿用海洋首领冷却与数量限制"}},
    {"2026年11月3日（金字塔地图）",
     {"新增金字塔地图花瓣，随机出现在金字塔可行走的地面",
      "每张金字塔地图同时放置24个，每30秒补充，地面保留10分钟",
      "走近即可拾取，存入背包后显示金字塔小地图，无需装备",
      "普通与转生金字塔均可拾取，地图花瓣随账号保存"}},
    {"2026年11月3日（深区刷怪）",
     {"仅提高电脑、下水道和金字塔现有深区的刷怪等级，浅区保持原样",
      "电脑最深区与下水道最深区基础S概率提高至约61%，下水道次深区约46%",
      "金字塔深区基础S概率约46%～61%，法老单体区约76%",
      "水母区保持此前全S配置，不随本次深区调整降低等级",
      "ET等首领仍沿用原有冷却和数量限制"}},
    {"2026年11月3日（水母区下调）",
     {"水母区较最初配置提升，调整为以U为主、约37%基础S的混合刷怪",
      "保留电脑、下水道和金字塔的深区提升"}},
    {"2026年11月3日（海洋尽头及其他深区）",
     {"提高海洋尽头、沙漠、花园和丛林原有最深区的刷怪等级",
      "海洋尽头、沙漠深区基础S概率约46%～61%，丛林最深区约61%",
      "花园三个深区基础S概率约41%、51%、61%",
      "水母区保留约37%基础S，浅区及其他地图沿用现有配置"}},
    {"2026年11月3日（搜索与输入修复）",
     {"背包和合成新增中英双语花瓣搜索，可按中文名、英文名或ID查找",
      "修复输入框在中间插入文字重复尾部、退格偶尔删除两次的问题",
      "分裂器落位改用完整身体碰撞路径，避免贴墙分裂后被怪物挤进墙内"}},
    {"2026年11月3日（瞄准箭头）",
     {"修复快速移动、点击鼠标时花朵移动/瞄准箭头随机跳转的问题",
      "真实鼠标事件保留原始坐标，脚本事件防护保持启用"}},
    {"2026年11月3日（精简血条）",
     {"满血S怪不再占用顶部首领血条，受伤后仍保留战斗血量提示",
      "顶部血条改为紧凑样式，最多两条，小屏幕最多一条，ET/Apex优先",
      "玩家自身及怪物身下的战斗血条照常保留"}},
    {"2026年11月3日（图鉴搜索）",
     {"图鉴新增怪物搜索，支持中文名、英文名及ID，与界面语言无关",
      "输入即筛选，保留击杀解锁与掉落提示，清空搜索可恢复完整列表"}},
    {"2026年11月5日（玩家反馈修复）",
     {"修复原地复活可能穿墙的问题，按实际身体大小沿可通行路径落点",
      "Apex自然升级概率由10%提高到20%，仍限制每个区域最多一只",
      "服务器怪物新增Cutter掉落，保留Digger原有掉落",
      "Ruby重复电击概率提升为80%至95%，额外电击上限提升为2至5次"}},
    {"2026年11月5日（掉落平衡）",
     {"普通怪掉落概率与主掉落品质升级概率各提高原值的15%，最高100%",
      "保持掉落数量、稀有度上限与额外低阶掉落规则不变"}},
    {"2026年11月5日（Cutter与水母区）",
     {"装备Cutter可与怪物重叠，不再互相挤开；接触伤害与墙壁碰撞保留",
      "水母区新增一个Apex名额，海洋最多同时两只，其余区域仍为一只"}},
    {"2026年11月6日（木桩与稀有怪刷新）",
     {"装备Cutter后可进入训练木桩内部测试DPS",
      "魔法萤火虫与金色叶虫的自然刷新权重各提高50%"}},
    {"2026年11月6日（垃圾堆外观）",
     {"修复垃圾堆移动时碎片反复换样的问题",
      "Apex垃圾堆使用更紧凑的碎片布局，减少巨型花瓣遮挡"}},
    {"2026年11月6日（天赋搜索）",
     {"天赋树支持中英文搜索，自动定位并突出显示匹配天赋",
      "按回车或点击下一个可在匹配分支之间切换"}},
    {"2026年11月6日（生物穿墙修复）",
     {"怪物移动、碰撞挤开与射击后坐力加入墙壁路径检查",
      "修复游走NPC和长身怪物的身体节段可能越过墙壁的问题"}},
    {"2026年11月7日（天赋与首领平衡）",
     {"110级起，每10级额外获得1点天赋点，已有角色自动补齐",
      "增强Reload天赋，Unique级的基础冷却倍率从29.2%降至27%",
      "水母区可补足海洋第3只Apex，普通水母刷新维持原样",
      "下水道Garbage从Super晋升Apex的判定概率由20%提高到30%"}},
    {"2026年11月7日（皮肤搜索与配装）",
     {"皮肤浏览支持按皮肤名或作者搜索",
      "控制设置新增当前配装预设自动保存，可轻点空预设创建",
      "分裂器的冷却随稀有度缩短，补全绷带、三叶草和电池的数值说明"}},
    {"2026年11月7日（测伤与合成）",
     {"修复训练木桩漏算 Ruby 的闪电连击",
      "电容器提高伤害上限，装备 Cutter 时花朵贴住敌人也能充电",
      "补全花朵与电容器的机制和数值说明",
      "提高普通合成的基础成功率，Common 升 Uncommon 从 64% 提至 72%"}},
    {"2026年11月8日（闪电联动）",
     {"电容器蓄电速度提高，短暂接触也能打出更多伤害",
      "闪电花瓣命中时，最强的电容器对一个目标追加电击，共享 0.9 秒冷却"}},
    {"2026年11月8日（电脑区与首领）",
     {"电脑区深处加入内核守卫，全服自然刷新最多一只",
      "提高电脑区 C++ 刷新权重，转换机每 15 分钟换一次配方",
      "普通地图的 Apex 同时存活上限提高到两只，水母区保留三个名额"}},
    {"2026年11月8日（电容器天赋）",
     {"电容器的持续放电间隔和闪电联动冷却享受一半 Reload 天赋缩减",
      "Common 电容器单次放电伤害上限调整为 75，联动追加伤害调整为 55"}},
    {"2026年11月9日（自动复活）",
     {"死亡后通常等待 8 秒自动复活，自动复活会等禁止复活效果结束",
      "点击继续仍可立即复活"}},
    {"2026年11月9日（Apex 掉落）",
     {"击败 Apex 怪物后，符合拾取条件的玩家可额外领取 1 个 Apex 花瓣"}},
    {"2026年11月9日（Apex 掉落平衡）",
     {"每只 Apex 怪独立判定，20% 概率额外掉落 1 个 Apex 花瓣"}},
    {"2026年11月10日（电容器重做）",
     {"电容器改为装备后常驻提高闪电花瓣的电击伤害，Ruby 连击也享受增伤",
      "Common 增伤 20%，每升一阶加 2 个百分点，Apex 增伤 38%；多个电容器取最高值"}},
    {"2026年11月10日（Hel 与电容器平衡）",
     {"Hel 东南新增高阶刷怪区，可自然刷新 Apex，血祭刷新间隔上限从 24 小时缩短至 8 小时",
      "电容器 Common 增伤 15%，每阶加 8 个百分点，Apex 87%，Universal 120%"}},
    {"2026年11月10日（顶点红宝石）",
     {"顶点 Ruby 对同一个目标的追加电击上限从 5 次提高到 7 次"}},
    {"2026年11月10日（复活、顶点怪与宇宙锻造）",
     {"普通地图和迷宫在死亡点复活，贴墙时会校正落点",
      "普通生物群系最多 3 只顶点怪，水母区最多 4 只",
      "顶点怪半径缩小 15%，优先在玩家附近安全刷新",
      "顶点怪血量 20 分钟不变会退场，无掉落",
      "宇宙花瓣的基础伤害和生命调为同款顶点的 5 倍",
      "同种宇宙花瓣可由 3 人同时持有，第 4 人锻造时替换最早持有者",
      "被替换时每件退还 4 个同种顶点花瓣"}},
    {"2026年11月11日（复活残影与顶点花瓣）",
     {"修复满血复活后旧尸体叠在新花朵上的问题",
      "顶点花瓣的基础伤害和生命各提高 25%，宇宙仍按顶点的 5 倍计算"}},
    {"2026年11月11日（金叶、击杀榜与冥界）",
     {"金叶每阶的单片减冷却从 2.5% 提高到 3%，顶点达到 30%",
      "排行榜可切换到累计怪物击杀榜，显示前 50 名",
      "冥界新增带毒的蜈蚣和会冲刺的赌徒",
      "冥界甲虫可额外掉落顶点血祭，究极至顶点分别为 2%、4%、6%、10%"}},
    {"2026年11月11日（红宝石掉落与蜈蚣追击）",
     {"电脑垃圾和服务器的红宝石掉落权重均提高一倍",
      "冥界蜈蚣的体节看见附近花朵时会脱离队列主动追击",
      "普通和声望冥界改为直接绘制地形，偶发黑屏待复测"}},
    {"2026年11月12日（冥界移速与私信刷新）",
     {"冥界蜈蚣在常规刷怪池中的加权占比从约 15% 提高到约 28%",
      "冥界怪的基础移速提高 20%，甲虫追击和赌徒冲刺也加快",
      "AQA 私信收件箱每 5 秒刷新，最新来信自动排在前面"}},
    {"2026年11月12日（宇宙血祭）",
     {"宇宙血祭现在会召唤宇宙怪物，长身怪的体节也保留宇宙稀有度",
      "召唤怪的基础生命和伤害为同种顶点的 2 倍，掉落沿用顶点规则"}},
};

constexpr int kEntryCount = static_cast<int>(sizeof(kChangelog) / sizeof(kChangelog[0]));

int changeCount(const ChangelogEntry& entry) {
    int count = 0;
    while (count < kMaxChanges && entry.changes[count] != nullptr) ++count;
    return count;
}

double entryHeight(const ChangelogEntry& entry) {
    return kDatePitch + kBulletPitch * changeCount(entry) + kEntryGap;
}

/// One clickable link, recorded while painting so the click pass has the same
/// geometry the glyphs were drawn at. Recorded UNCLIPPED, as the reference
/// does; `render` gates the hit on the content rect instead.
struct LinkHit {
    Rect rect;
    std::string url;
};

/// A run of a change line: literal text, or a link when `url` is set.
struct Segment {
    std::string text;
    std::string url;
};

/// Thumb-drag state for the scrollbar.
///
/// At file scope because it lives entirely between one press and the release
/// that ends it, and because the panel's own declaration sits in a header
/// every panel shares -- widening it for one panel's transient would cost
/// every other panel a recompile and tell a reader nothing.
struct ThumbDrag {
    bool active = false;
    double startY = 0;
    double startOffset = 0;
};

ThumbDrag& thumbDrag() {
    static ThumbDrag drag;
    return drag;
}

bool isSpaceByte(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

bool hasScheme(const std::string& url) {
    const auto starts = [&url](const char* prefix) {
        const std::size_t n = std::char_traits<char>::length(prefix);
        if (url.size() < n) return false;
        for (std::size_t i = 0; i < n; ++i) {
            if (std::tolower(static_cast<unsigned char>(url[i])) != prefix[i]) return false;
        }
        return true;
    };
    return starts("http://") || starts("https://");
}

std::string normalizeUrl(const std::string& raw) {
    return hasScheme(raw) ? raw : "https://" + raw;
}

/// `new URL(u).host + pathname`, with a trailing slash trimmed: a link is
/// shown by where it goes, not by how it was written.
std::string linkLabel(const std::string& raw) {
    const std::string url = normalizeUrl(raw);
    const std::size_t afterScheme = url.find("//");
    std::string rest = afterScheme == std::string::npos ? url : url.substr(afterScheme + 2);
    // Neither the query nor the fragment is part of a host or a pathname.
    rest = rest.substr(0, rest.find_first_of("?#"));
    if (rest.size() > 1 && rest.back() == '/') rest.pop_back();
    return rest;
}

/// Splits a change line on `link:<token>`, exactly as the reference's regex
/// does. A bare "link:" with no token after it is not a match and stays text.
std::vector<Segment> parseChange(const std::string& change) {
    std::vector<Segment> segments;
    std::size_t last = 0;
    std::size_t from = 0;
    while (true) {
        const std::size_t at = change.find("link:", from);
        if (at == std::string::npos) break;
        std::size_t end = at + 5;
        while (end < change.size() && !isSpaceByte(change[end])) ++end;
        if (end == at + 5) {
            from = end;
            continue;
        }
        if (at > last) segments.push_back({change.substr(last, at - last), {}});
        const std::string raw = change.substr(at + 5, end - at - 5);
        segments.push_back({linkLabel(raw), normalizeUrl(raw)});
        last = from = end;
    }
    if (last < change.size()) segments.push_back({change.substr(last), {}});
    if (segments.empty()) segments.push_back({change, {}});
    return segments;
}

/// The one glyph in the build that is filled and THEN stroked.
///
/// The reference paints the bullet with fillText followed by strokeText, so
/// its black outline sits ON TOP of the white dot instead of behind it. At
/// 14px that outline is most of what the bullet looks like, which is why the
/// order is reproduced rather than routed through ui::text().
void drawBullet(Canvas& canvas, double x, double y, double strokeWidth) {
    TextStyle style;
    style.size = kChangeSize;
    style.fill = kPaper;
    style.stroke = kInk;
    // Clamped, not passed through: a negative strokeWidth means "derive one"
    // to paintRun, where here it has always meant "no outline".
    style.strokeWidth = std::max(0.0, strokeWidth);
    paintRun(canvas, "•", x, y + (ascent(kChangeSize) + descent(kChangeSize)) * 0.5, style,
             1.0, 1.0, /*fillFirst=*/true);
}

/// Paints one change line. `strokeWidth` is the BULLET outline, threaded by
/// reference because a link underline leaves it at 1 for every bullet after
/// it, which the reference does by leaking ctx.lineWidth. The line's own text
/// takes the shared outline.
void drawChange(Canvas& canvas, const std::string& change, double x, double y,
                double& strokeWidth, std::vector<LinkHit>& links) {
    double penX = x;
    for (const Segment& segment : parseChange(change)) {
        if (segment.text.empty()) continue;
        const double width = measure(segment.text, kChangeSize);

        TextStyle style;
        style.size = kChangeSize;
        style.fill = segment.url.empty() ? kPaper : kLinkFill;
        text(canvas, segment.text, penX, y, style);

        if (!segment.url.empty()) {
            canvas.save();
            setStroke(canvas, kLinkFill);
            canvas.setLineWidth(1.0f);
            canvas.beginPath();
            canvas.moveTo(static_cast<float>(penX), static_cast<float>(y + 17.0));
            canvas.lineTo(static_cast<float>(penX + width), static_cast<float>(y + 17.0));
            canvas.stroke();
            canvas.restore();
            strokeWidth = 1.0;
            links.push_back({Rect{penX, y, width, 18.0}, segment.url});
        }
        penX += width;
    }
}

} // namespace

int changelogReleaseCount() { return kEntryCount; }

double ChangelogPanel::preferredWidth() { return 600.0; }

void ChangelogPanel::reset() {
    scroll_ = {};
    thumbDrag() = {};
}

bool ChangelogPanel::render(MenuContext& ctx) {
    Canvas& canvas = ctx.canvas;
    const Rect panel = ctx.bounds;
    const Vec2 mouse = ctx.mouse();

    const double viewport = panel.h - kHeaderHeight;
    double contentHeight = 0;
    for (const ChangelogEntry& entry : kChangelog) contentHeight += entryHeight(entry);
    scroll_.contentHeight = contentHeight;
    scroll_.viewHeight = viewport;
    const double maxScroll = scroll_.maxOffset();
    const bool scrollable = contentHeight > viewport;

    const Rect track{panel.right() - kScrollbarWidth - 5.0, panel.y + kHeaderHeight,
                     kScrollbarWidth, panel.h - kHeaderHeight - 5.0};

    // The wheel is INVERTED here: in the reference a notch DOWN walks the list
    // back toward the newest entry. Odd, deliberate there, kept here.
    if (panel.contains(mouse)) scroll_.offset += ctx.wheel() * kWheelStep;
    // A finger is NOT inverted: the text follows it, whichever way the wheel
    // goes. Everything under the heading but the thumb's track, which a finger
    // drags the other way.
    const Rect body{panel.x, panel.y + kHeaderHeight, track.x - panel.x, panel.h - kHeaderHeight};
    scroll_.offset -= touchScroll(ctx.window, body, scrollable);

    ThumbDrag& drag = thumbDrag();
    if (ctx.pressed() && scrollable && track.contains(mouse)) {
        drag.active = true;
        drag.startY = mouse.y;
        drag.startOffset = scroll_.offset;
    }
    if (!ctx.window.mouseDown(MouseButton::Left)) drag.active = false;
    if (drag.active) {
        // The reference maps the drag over panelHeight - 45, not over the
        // track: the thumb runs slightly ahead of the cursor by design.
        const double ratio = (mouse.y - drag.startY) / (panel.h - 45.0);
        scroll_.offset = drag.startOffset + ratio * maxScroll;
    }
    scroll_.offset = clamp(scroll_.offset, 0.0, maxScroll);

    panelCard(canvas, panel, kChangelogSkin);

    TextStyle heading;
    heading.size = 20.0;
    heading.fill = kPaper;
    heading.baseline = Baseline::Top;
    text(canvas, menuTitle(MenuId::Changelog), panel.x + kPadding, panel.y + kPadding, heading);

    // No hover and no press state, exactly as the reference pill has none.
    const Rect closeRect = closeButtonRect(panel);
    panelClose(canvas, closeRect, closeRect.contains(mouse));

    const Rect view{panel.x + kPadding, panel.y + kHeaderHeight, panel.w - kPadding * 2,
                    panel.h - kHeaderHeight - kPadding};

    canvas.save();
    roundPath(canvas, view, 8.0);
    canvas.clip();

    // The date strip yields its right edge to the scrollbar when there is one.
    const double stripWidth = view.w - (scrollable ? kScrollbarWidth + 5.0 : 0.0);
    double contentY = view.y + kPadding - scroll_.offset;
    std::vector<LinkHit> links;

    // Newest first: the table is stored oldest-first and read backwards.
    for (int i = kEntryCount - 1; i >= 0; --i) {
        const ChangelogEntry& entry = kChangelog[i];
        const double height = entryHeight(entry);
        // A row's glyphs reach past its pitch, so the cull keeps a line of
        // slack on each side rather than trusting the box.
        if (contentY + height < view.y - kBulletPitch || contentY > view.bottom() + kBulletPitch) {
            contentY += height;
            continue;
        }

        setFill(canvas, kPaper, 0.05);
        roundPath(canvas, Rect{view.x, contentY - 5.0, stripWidth, 10.0}, 8.0);
        canvas.fill();

        TextStyle date;
        date.size = 20.0;
        date.fill = kPaper;
        text(canvas, entry.date, view.x, contentY, date);
        contentY += kDatePitch;

        // The first bullet of an entry is outlined at 1px and the rest at
        // 0.5, which is what makes the top bullet of every group read darker.
        double strokeWidth = 1.0;
        const int changes = changeCount(entry);
        for (int c = 0; c < changes; ++c) {
            drawBullet(canvas, view.x, contentY, strokeWidth);
            strokeWidth = 0.5;
            drawChange(canvas, entry.changes[c], view.x + kBulletIndent, contentY, strokeWidth,
                       links);
            contentY += kBulletPitch;
        }
        contentY += kEntryGap;
    }
    canvas.restore();

    if (scrollable) {
        setFill(canvas, kPaper, 0.1);
        roundPath(canvas, track, 5.0);
        canvas.fill();
        // No minimum thumb height: on a 7896px list the reference's thumb is
        // 26px, and clamping it to a comfortable size would move it off the
        // scroll position it is meant to report.
        const double thumbHeight = (viewport - 5.0) * viewport / contentHeight;
        const double travelled = maxScroll > 0 ? scroll_.offset / maxScroll : 0.0;
        setFill(canvas, kBodyBorder);
        roundPath(canvas,
                      Rect{track.x, track.y + travelled * (track.h - thumbHeight),
                           track.w, thumbHeight},
                      5.0);
        canvas.fill();
    }

    if (!ctx.released()) return true;
    if (closeRect.contains(mouse)) return false;
    // Link rects are recorded unclipped, so the hit is gated on the body the
    // way the reference gates it.
    if (view.contains(mouse)) {
        for (const LinkHit& link : links) {
            if (!link.rect.contains(mouse)) continue;
            openExternalLink(link.url);
            break;
        }
    }
    return true;
}

} // namespace flix
