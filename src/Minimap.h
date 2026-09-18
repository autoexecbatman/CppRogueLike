#pragma once

struct GameContext;

class Minimap
{
    // How much of the window the overlay may take across and down. The pixels per
    // map tile are worked out from this and the map's own size, because a fixed
    // two pixels a tile made a 120x80 map a 240x160 postage stamp in which a single
    // explored room was five pixels across.
    static constexpr int MAX_WIDTH_PERCENT = 40;
    static constexpr int MAX_HEIGHT_PERCENT = 55;
    static constexpr int PADDING = 8;
    static constexpr int BORDER = 3;

    bool visible{ false };

public:
    void toggle() noexcept;
    [[nodiscard]] bool is_visible() const noexcept;
    void render(const GameContext& ctx) const;
};
