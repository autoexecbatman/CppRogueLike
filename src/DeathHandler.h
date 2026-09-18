#pragma once

class Creature;
struct GameContext;

// Type discriminator used only for serialization round-trips.
// Lives here because DeathHandler is the only reason this distinction exists.
enum class DestructibleType
{
    MONSTER,
    PLAYER
};

// DIP: Destructible depends on this abstraction, not on Player or Creature concretions.
// Named type (not std::function) so stack traces and debuggers show a meaningful name.
struct DeathHandler
{
    virtual ~DeathHandler() = default;
    virtual void execute(Creature& owner, GameContext& ctx) = 0;
    [[nodiscard]] virtual DestructibleType type() const = 0;
};

struct MonsterDeathHandler : DeathHandler
{
    void execute(Creature& owner, GameContext& ctx) override;
    [[nodiscard]] DestructibleType type() const override;
};

struct PlayerDeathHandler : DeathHandler
{
    void execute(Creature& owner, GameContext& ctx) override;
    [[nodiscard]] DestructibleType type() const override;
};
