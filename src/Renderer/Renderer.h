#pragma once

#include <string_view>
#include <array>
#include <raylib.h>

// Undefine raylib color macros that conflict with game enum values
// (e.g. ItemId::GOLD, ItemClass::GOLD, PickableType::GOLD)
#undef LIGHTGRAY
#undef GRAY
#undef DARKGRAY
#undef YELLOW
#undef GOLD
#undef ORANGE
#undef PINK
#undef RED
#undef MAROON
#undef GREEN
#undef LIME
#undef DARKGREEN
#undef SKYBLUE
#undef BLUE
#undef DARKBLUE
#undef PURPLE
#undef VIOLET
#undef DARKPURPLE
#undef BEIGE
#undef BROWN
#undef DARKBROWN
#undef WHITE
#undef BLACK
#undef BLANK
#undef MAGENTA
#undef RAYWHITE

struct ScreenMetrics
{
    int tile_w;
    int tile_h;
    int map_cols;
    int map_rows;
    int gui_rows;
    int window_w;
    int window_h;
};

struct ColorPair
{
    Color fg;
    Color bg;
};

inline constexpr int MAX_COLOR_PAIRS = 23;
inline constexpr int DEFAULT_TILE_SIZE = 16;
inline constexpr int DEFAULT_WINDOW_COLS = 119;
inline constexpr int DEFAULT_WINDOW_ROWS = 30;
inline constexpr int GUI_RESERVE_ROWS = 8;

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void init(int cols, int rows);
    void shutdown();

    void load_tilesets(const char* frame0_path, const char* frame1_path);
    void load_font(const char* font_path, int font_size);

    void begin_frame();
    void end_frame();

    void draw_tile(int grid_y, int grid_x, int cp437_code, int color_pair_id) const;
    void draw_text(int px, int py, std::string_view text, int color_pair_id) const;
    void draw_bar(int px, int py, int w, int h, float ratio, Color filled, Color empty) const;

    [[nodiscard]] ColorPair get_color_pair(int id) const;
    [[nodiscard]] ScreenMetrics metrics() const;

    [[nodiscard]] bool is_initialized() const { return initialized; }
    [[nodiscard]] int get_tile_size() const { return tile_size; }

private:
    void init_color_pairs();

    bool initialized{ false };
    int tile_size{ DEFAULT_TILE_SIZE };
    int window_cols{ DEFAULT_WINDOW_COLS };
    int window_rows{ DEFAULT_WINDOW_ROWS };

    Texture2D tileset_frame0{};
    Texture2D tileset_frame1{};
    bool tilesets_loaded{ false };

    Font game_font{};
    bool font_loaded{ false };
    int font_size{ 16 };

    int current_anim_frame{ 0 };
    double last_anim_toggle{ 0.0 };
    static constexpr double ANIM_INTERVAL = 0.5;

    std::array<ColorPair, MAX_COLOR_PAIRS> color_pairs{};
};
