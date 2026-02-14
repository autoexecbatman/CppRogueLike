// file: Renderer.cpp
#include "Renderer.h"
#include <format>
#include <string>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

// Local color constants (raylib macros are #undef'd in Renderer.h)
static constexpr Color RL_WHITE   = { 255, 255, 255, 255 };
static constexpr Color RL_BLACK   = { 0, 0, 0, 255 };
static constexpr Color RL_RED     = { 230, 41, 55, 255 };
static constexpr Color RL_GREEN   = { 0, 228, 48, 255 };
static constexpr Color RL_BLUE    = { 0, 121, 241, 255 };
static constexpr Color RL_YELLOW  = { 253, 249, 0, 255 };
static constexpr Color RL_MAGENTA = { 255, 0, 255, 255 };

void Renderer::init(int cols, int rows)
{
    window_cols = cols;
    window_rows = rows;

    int pixel_w = cols * tile_size;
    int pixel_h = rows * tile_size;

    InitWindow(pixel_w, pixel_h, "C++RogueLike");
    SetTargetFPS(60);
    SetExitKey(0);

    init_color_pairs();
    initialized = true;
}

void Renderer::shutdown()
{
    if (tilesets_loaded)
    {
        UnloadTexture(tileset_frame0);
        UnloadTexture(tileset_frame1);
        tilesets_loaded = false;
    }

    if (font_loaded)
    {
        UnloadFont(game_font);
        font_loaded = false;
    }

    if (initialized)
    {
        CloseWindow();
        initialized = false;
    }
}

void Renderer::load_tilesets(const char* frame0_path, const char* frame1_path)
{
    tileset_frame0 = LoadTexture(frame0_path);
    tileset_frame1 = LoadTexture(frame1_path);
    tilesets_loaded = (tileset_frame0.id > 0 && tileset_frame1.id > 0);
}

void Renderer::load_font(const char* font_path, int size)
{
    font_size = size;
    game_font = LoadFontEx(font_path, size, nullptr, 256);
    font_loaded = (game_font.glyphCount > 0);
}

void Renderer::begin_frame()
{
    double now = GetTime();
    if (now - last_anim_toggle >= ANIM_INTERVAL)
    {
        current_anim_frame = 1 - current_anim_frame;
        last_anim_toggle = now;
    }

    BeginDrawing();
    ClearBackground(RL_BLACK);
}

void Renderer::end_frame()
{
    EndDrawing();
#ifdef EMSCRIPTEN
    emscripten_sleep(0);
#endif
}

void Renderer::draw_tile(int grid_y, int grid_x, int cp437_code, int color_pair_id) const
{
    if (!tilesets_loaded)
    {
        return;
    }

    const Texture2D& tileset = (current_anim_frame == 0) ? tileset_frame0 : tileset_frame1;

    int tiles_per_row = tileset.width / tile_size;
    int src_x = (cp437_code % tiles_per_row) * tile_size;
    int src_y = (cp437_code / tiles_per_row) * tile_size;

    Rectangle src_rect = {
        static_cast<float>(src_x),
        static_cast<float>(src_y),
        static_cast<float>(tile_size),
        static_cast<float>(tile_size)
    };

    Vector2 dest = {
        static_cast<float>(grid_x * tile_size),
        static_cast<float>(grid_y * tile_size)
    };

    ColorPair pair = get_color_pair(color_pair_id);
    DrawTextureRec(tileset, src_rect, dest, pair.fg);
}

void Renderer::draw_text(int px, int py, std::string_view text, int color_pair_id) const
{
    ColorPair pair = get_color_pair(color_pair_id);
    std::string text_str(text);

    if (font_loaded)
    {
        Vector2 pos = { static_cast<float>(px), static_cast<float>(py) };
        DrawTextEx(game_font, text_str.c_str(), pos, static_cast<float>(font_size), 1.0f, pair.fg);
    }
    else
    {
        DrawText(text_str.c_str(), px, py, font_size, pair.fg);
    }
}

void Renderer::draw_bar(int px, int py, int w, int h, float ratio, Color filled, Color empty) const
{
    DrawRectangle(px, py, w, h, empty);

    int filled_w = static_cast<int>(static_cast<float>(w) * ratio);
    if (filled_w > 0)
    {
        DrawRectangle(px, py, filled_w, h, filled);
    }
}

ColorPair Renderer::get_color_pair(int id) const
{
    if (id >= 0 && id < MAX_COLOR_PAIRS)
    {
        return color_pairs[id];
    }
    return ColorPair{ RL_WHITE, RL_BLACK };
}

ScreenMetrics Renderer::metrics() const
{
    return ScreenMetrics{
        .tile_w = tile_size,
        .tile_h = tile_size,
        .map_cols = window_cols,
        .map_rows = window_rows - GUI_RESERVE_ROWS,
        .gui_rows = GUI_RESERVE_ROWS,
        .window_w = window_cols * tile_size,
        .window_h = window_rows * tile_size
    };
}

void Renderer::init_color_pairs()
{
    // Default: white on black
    for (auto& pair : color_pairs)
    {
        pair = ColorPair{ RL_WHITE, RL_BLACK };
    }

    // === WHITE FOREGROUND PAIRS ===
    color_pairs[1] = ColorPair{ RL_WHITE, RL_BLACK };
    color_pairs[2] = ColorPair{ RL_WHITE, RL_RED };
    color_pairs[3] = ColorPair{ RL_WHITE, RL_BLUE };
    color_pairs[4] = ColorPair{ RL_WHITE, Color{ 0, 102, 0, 255 } };  // dim green bg

    // === BLACK FOREGROUND PAIRS ===
    color_pairs[5] = ColorPair{ RL_BLACK, RL_WHITE };
    color_pairs[6] = ColorPair{ RL_BLACK, RL_GREEN };
    color_pairs[7] = ColorPair{ RL_BLACK, RL_YELLOW };
    color_pairs[8] = ColorPair{ RL_BLACK, RL_RED };

    // === COLORED FOREGROUND ON BLACK ===
    color_pairs[9]  = ColorPair{ RL_RED, RL_BLACK };
    color_pairs[10] = ColorPair{ RL_GREEN, RL_BLACK };
    color_pairs[11] = ColorPair{ RL_YELLOW, RL_BLACK };
    color_pairs[12] = ColorPair{ RL_BLUE, RL_BLACK };
    color_pairs[13] = ColorPair{ Color{ 0, 255, 255, 255 }, RL_BLACK };   // cyan
    color_pairs[14] = ColorPair{ RL_MAGENTA, RL_BLACK };

    // === SPECIAL COMBINATIONS ===
    color_pairs[15] = ColorPair{ Color{ 0, 255, 255, 255 }, RL_BLUE };    // cyan on blue
    color_pairs[16] = ColorPair{ RL_RED, RL_WHITE };
    color_pairs[17] = ColorPair{ RL_GREEN, RL_YELLOW };
    color_pairs[18] = ColorPair{ RL_GREEN, RL_MAGENTA };
    color_pairs[19] = ColorPair{ RL_RED, RL_YELLOW };
    color_pairs[20] = ColorPair{ RL_GREEN, RL_RED };

    // === CUSTOM COLORS ===
    color_pairs[21] = ColorPair{ Color{ 128, 77, 0, 255 }, RL_BLACK };    // brown
    color_pairs[22] = ColorPair{ Color{ 0, 102, 0, 255 }, RL_BLACK };     // dim green
}

// end of file: Renderer.cpp
