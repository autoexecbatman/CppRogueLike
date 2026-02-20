#pragma once

#include <string_view>
#include <array>
#include <raylib.h>

#include "TileId.h"

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
inline constexpr int SPRITE_SIZE = 16;       // DawnLike native sprite pixel size
inline constexpr int DISPLAY_TILE_SIZE = 32;  // On-screen tile pixel size (2x)
inline constexpr int GUI_RESERVE_ROWS = 7;

// Holds one DawnLike sprite sheet (optionally two frames for animation).
struct SpriteSheet
{
    Texture2D frame0{};
    Texture2D frame1{};
    int tiles_per_row{ 0 };
    bool animated{ false };
    bool loaded{ false };
};

struct GameCamera
{
    int x{ 0 };
    int y{ 0 };
};

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void init();
    void shutdown();

    void load_dawnlike(const char* base_path);
    void load_font(const char* font_path, int size);

    void begin_frame();
    void end_frame();

    // World-space tile drawing (camera offset applied)
    void draw_tile(int grid_y, int grid_x, int tile_id, int color_pair_id) const;

    // Screen-space drawing (no camera offset)
    void draw_tile_screen(int px, int py, int tile_id) const;
    void draw_text(int px, int py, std::string_view text, int color_pair_id) const;
    void draw_bar(int px, int py, int w, int h, float ratio, Color filled, Color empty) const;

    // Draw a DawnLike-tiled frame with dark background fill.
    // px/py = top-left in pixels; w_tiles/h_tiles = dimensions in tiles.
    void draw_frame(int px, int py, int w_tiles, int h_tiles) const;

    void set_camera_center(int world_tile_x, int world_tile_y, int map_w, int map_h);

    [[nodiscard]] ColorPair get_color_pair(int id) const;
    [[nodiscard]] ScreenMetrics metrics() const;
    [[nodiscard]] int measure_text(std::string_view text) const;

    [[nodiscard]] bool is_initialized() const { return initialized; }
    [[nodiscard]] int get_tile_size() const { return tile_size; }
    [[nodiscard]] int get_font_size() const { return font_size; }
    [[nodiscard]] int get_viewport_cols() const { return viewport_cols; }
    [[nodiscard]] int get_viewport_rows() const { return viewport_rows; }
    [[nodiscard]] int get_screen_width() const { return screen_w; }
    [[nodiscard]] int get_screen_height() const { return screen_h; }
    [[nodiscard]] int get_camera_x() const { return camera.x; }
    [[nodiscard]] int get_camera_y() const { return camera.y; }

private:
    void init_color_pairs();
    void load_sheet(int sheet_id, const char* path0, const char* path1);
    void load_sheet_static(int sheet_id, const char* path);

    bool initialized{ false };
    int tile_size{ DISPLAY_TILE_SIZE };
    int viewport_cols{ 0 };
    int viewport_rows{ 0 };
    int screen_w{ 0 };
    int screen_h{ 0 };

    GameCamera camera{};

    std::array<SpriteSheet, SHEET_COUNT> sheets{};
    bool sheets_loaded{ false };

    Font game_font{};
    bool font_loaded{ false };
    int font_size{ 24 };

    int current_anim_frame{ 0 };
    double last_anim_toggle{ 0.0 };
    static constexpr double ANIM_INTERVAL = 0.5;

    std::array<ColorPair, MAX_COLOR_PAIRS> color_pairs{};
};
