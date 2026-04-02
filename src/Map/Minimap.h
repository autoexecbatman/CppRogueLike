#pragma once

struct GameContext;

class Minimap
{
    static constexpr int TILE_PX = 2;
    static constexpr int PADDING = 8;

    bool visible{ false };

public:
    void toggle() noexcept;
    [[nodiscard]] bool is_visible() const noexcept;
    void render(const GameContext& ctx) const;
};
