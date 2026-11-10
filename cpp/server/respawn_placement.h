#pragma once
#include "server/systems/movement.h"

namespace flix {
/// Keep the death position unless the restored body needs clearance from terrain.
inline Vec2 respawnPosition(const Terrain& terrain, Realm realm, Vec2 corpse, double bodyRadius) {
    return terrain.resolveCircle(corpse, bodyRadius, realm);
}
} // namespace flix
