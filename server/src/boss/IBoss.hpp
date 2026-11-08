#pragma once

#include "GameContext.hpp"
#include "LevelData.hpp"
#include "boss/BossStatus.hpp"

namespace rtype::server {

class IBoss
{
public:
    virtual ~IBoss() = default;

    virtual void spawn(GameContext& ctx, const level::BossDef& def, int health) = 0;
    virtual void update(GameContext& ctx, float dt) = 0;
    virtual bool defeated() const = 0;
    virtual BossStatus status() const = 0;
};

} // namespace rtype::server
