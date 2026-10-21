#pragma once

#include "GameContext.hpp"
#include "IGameMode.hpp"
#include "ISystem.hpp"

namespace rtype::server {

class ModeUpdateSystem : public engine::ISystem
{
public:
    ModeUpdateSystem(IGameMode* const& mode, GameContext& ctx) : _mode(mode), _ctx(ctx) {}
    void update(engine::Registry& /*registry*/, float dt) override
    {
        if (_mode)
            _mode->update(_ctx, dt);
    }

private:
    IGameMode* const& _mode;
    GameContext& _ctx;
};

class ModeResolveSystem : public engine::ISystem
{
public:
    ModeResolveSystem(IGameMode* const& mode, GameContext& ctx) : _mode(mode), _ctx(ctx) {}
    void update(engine::Registry& /*registry*/, float dt) override
    {
        if (_mode)
            _mode->resolve(_ctx, dt);
    }

private:
    IGameMode* const& _mode;
    GameContext& _ctx;
};

} // namespace rtype::server
