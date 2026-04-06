// file: Systems/TargetingMenu.h
// Frame-based targeting cursor menu. Replaces the blocking run_targeting_loop.
// Push onto ctx.menus; fires onComplete(confirmed, position, ctx) when user confirms or cancels.
#pragma once

#include <functional>

#include "../Menu/BaseMenu.h"
#include "../Utils/Vector2D.h"

struct GameContext;

class TargetingMenu : public BaseMenu
{
public:
    using Callback = std::function<void(bool confirmed, Vector2D position, GameContext&)>;

    TargetingMenu(int maxRange, int aoeRadius, Callback onComplete, GameContext& ctx);
    ~TargetingMenu() = default;
    TargetingMenu(const TargetingMenu&) = delete;
    TargetingMenu& operator=(const TargetingMenu&) = delete;
    TargetingMenu(TargetingMenu&&) = delete;
    TargetingMenu& operator=(TargetingMenu&&) = delete;

    void menu(GameContext& ctx) override;

private:
    Vector2D cursor{};
    int maxRange{};
    int aoeRadius{};
    Callback onComplete;

    void draw_cursor(GameContext& ctx) const;
};

// end of file: Systems/TargetingMenu.h
