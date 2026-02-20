// file: Renderer.cpp
#include "Renderer.h"
#include <format>
#include <string>
#include <algorithm>

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

// DawnLike uses magenta (255,0,255) as the transparency key.
// Load a PNG, replace magenta pixels with alpha=0, return as Texture2D.
static Texture2D load_dawnlike_texture(const char* path)
{
    Image img = LoadImage(path);
    if (img.data == nullptr)
    {
        return Texture2D{};
    }
    ImageColorReplace(&img, RL_MAGENTA, Color{ 0, 0, 0, 0 });
    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

void Renderer::init()
{
#ifdef EMSCRIPTEN
    screen_w = 960;
    screen_h = 544;
    InitWindow(screen_w, screen_h, "C++RogueLike");
    SetTargetFPS(60);
#else
    // Create initial window to query monitor
    InitWindow(800, 600, "C++RogueLike");
    SetTargetFPS(60);
    SetExitKey(0);

    int monitor = GetCurrentMonitor();
    screen_w = GetMonitorWidth(monitor);
    screen_h = GetMonitorHeight(monitor);

    // Go fullscreen at monitor resolution
    SetWindowSize(screen_w, screen_h);
    SetWindowPosition(0, 0);
    ToggleFullscreen();
#endif

    // Viewport in tile units
    viewport_cols = screen_w / tile_size;
    viewport_rows = screen_h / tile_size;

    // Font scaled proportionally to tile size
    font_size = tile_size * 3 / 4;

    init_color_pairs();
    initialized = true;
}

void Renderer::shutdown()
{
    for (auto& sheet : sheets)
    {
        if (sheet.loaded)
        {
            UnloadTexture(sheet.frame0);
            if (sheet.animated)
            {
                UnloadTexture(sheet.frame1);
            }
            sheet.loaded = false;
        }
    }
    sheets_loaded = false;

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

void Renderer::load_sheet(int sheet_id, const char* path0, const char* path1)
{
    auto& s = sheets[sheet_id];
    s.frame0 = load_dawnlike_texture(path0);
    s.frame1 = load_dawnlike_texture(path1);
    s.tiles_per_row = s.frame0.width / SPRITE_SIZE;
    s.animated = true;
    s.loaded = (s.frame0.id > 0);
}

void Renderer::load_sheet_static(int sheet_id, const char* path)
{
    auto& s = sheets[sheet_id];
    s.frame0 = load_dawnlike_texture(path);
    s.frame1 = s.frame0;
    s.tiles_per_row = s.frame0.width / SPRITE_SIZE;
    s.animated = false;
    s.loaded = (s.frame0.id > 0);
}

void Renderer::load_dawnlike(const char* base_path)
{
    std::string base(base_path);
    if (!base.empty() && base.back() != '/')
    {
        base += '/';
    }

    auto load_animated = [&](int id, const char* dir, const char* name)
    {
        std::string p0 = std::format("{}{}{}0.png", base, dir, name);
        std::string p1 = std::format("{}{}{}1.png", base, dir, name);
        load_sheet(id, p0.c_str(), p1.c_str());
    };

    auto load_static = [&](int id, const char* dir, const char* name)
    {
        std::string p = std::format("{}{}{}.png", base, dir, name);
        load_sheet_static(id, p.c_str());
    };

    // Objects
    load_static(SHEET_FLOOR, "Objects/", "Floor");
    load_static(SHEET_WALL, "Objects/", "Wall");
    load_static(SHEET_DOOR0, "Objects/", "Door0");
    load_animated(SHEET_DECOR0, "Objects/", "Decor");
    load_animated(SHEET_EFFECT0, "Objects/", "Effect");
    load_static(SHEET_TILE, "Objects/", "Tile");
    load_animated(SHEET_PIT0, "Objects/", "Pit");
    load_animated(SHEET_GUI0, "GUI/", "GUI");

    // Characters (all animated with 0/1 pairs)
    load_animated(SHEET_PLAYER0, "Characters/", "Player");
    load_animated(SHEET_HUMANOID0, "Characters/", "Humanoid");
    load_animated(SHEET_REPTILE0, "Characters/", "Reptile");
    load_animated(SHEET_PEST0, "Characters/", "Pest");
    load_animated(SHEET_DOG0, "Characters/", "Dog");
    load_animated(SHEET_AVIAN0, "Characters/", "Avian");
    load_animated(SHEET_UNDEAD0, "Characters/", "Undead");
    load_animated(SHEET_QUADRAPED0, "Characters/", "Quadraped");
    load_animated(SHEET_DEMON0, "Characters/", "Demon");
    load_animated(SHEET_MISC0, "Characters/", "Misc");

    // Items (static -- no animation frames)
    load_static(SHEET_POTION, "Items/", "Potion");
    load_static(SHEET_SCROLL, "Items/", "Scroll");
    load_static(SHEET_SHORT_WEP, "Items/", "ShortWep");
    load_static(SHEET_MED_WEP, "Items/", "MedWep");
    load_static(SHEET_LONG_WEP, "Items/", "LongWep");
    load_static(SHEET_ARMOR, "Items/", "Armor");
    load_static(SHEET_SHIELD, "Items/", "Shield");
    load_static(SHEET_HAT, "Items/", "Hat");
    load_static(SHEET_RING, "Items/", "Ring");
    load_static(SHEET_AMULET_ITEM, "Items/", "Amulet");
    load_static(SHEET_FOOD, "Items/", "Food");
    load_static(SHEET_FLESH, "Items/", "Flesh");
    load_static(SHEET_MONEY, "Items/", "Money");

    sheets_loaded = true;
}

void Renderer::load_font(const char* font_path, int size)
{
    font_size = size;
    game_font = LoadFontEx(font_path, size, nullptr, 256);
    font_loaded = (game_font.glyphCount > 0);
    if (font_loaded)
    {
        SetTextureFilter(game_font.texture, TEXTURE_FILTER_POINT);
    }
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

void Renderer::draw_tile(int grid_y, int grid_x, int tile_id, int /*color_pair_id*/) const
{
    if (!sheets_loaded)
    {
        return;
    }

    int sid = tile_sheet(tile_id);
    if (sid < 0 || sid >= SHEET_COUNT)
    {
        return;
    }

    const auto& sheet = sheets[sid];
    if (!sheet.loaded || sheet.tiles_per_row <= 0)
    {
        return;
    }

    // Screen position with camera offset
    float dest_x = static_cast<float>(grid_x * tile_size - camera.x);
    float dest_y = static_cast<float>(grid_y * tile_size - camera.y);

    // Cull tiles outside the visible area
    float ts_f = static_cast<float>(tile_size);
    if (dest_x + ts_f < 0.0f || dest_x >= static_cast<float>(screen_w) ||
        dest_y + ts_f < 0.0f || dest_y >= static_cast<float>(screen_h))
    {
        return;
    }

    const Texture2D& tex = (sheet.animated && current_anim_frame == 1)
        ? sheet.frame1
        : sheet.frame0;

    int col = tile_col(tile_id);
    int row = tile_row(tile_id);

    // Source rect in the 16x16 sprite sheet
    Rectangle src_rect = {
        static_cast<float>(col * SPRITE_SIZE),
        static_cast<float>(row * SPRITE_SIZE),
        static_cast<float>(SPRITE_SIZE),
        static_cast<float>(SPRITE_SIZE)
    };

    // Destination rect scaled to display tile size
    Rectangle dest_rect = { dest_x, dest_y, ts_f, ts_f };

    DrawTexturePro(tex, src_rect, dest_rect, { 0.0f, 0.0f }, 0.0f, RL_WHITE);
}

void Renderer::draw_tile_screen(int px, int py, int tile_id) const
{
    if (!sheets_loaded)
    {
        return;
    }

    int sid = tile_sheet(tile_id);
    if (sid < 0 || sid >= SHEET_COUNT)
    {
        return;
    }

    const auto& sheet = sheets[sid];
    if (!sheet.loaded || sheet.tiles_per_row <= 0)
    {
        return;
    }

    const Texture2D& tex = (sheet.animated && current_anim_frame == 1)
        ? sheet.frame1
        : sheet.frame0;

    int col = tile_col(tile_id);
    int row = tile_row(tile_id);

    Rectangle src_rect = {
        static_cast<float>(col * SPRITE_SIZE),
        static_cast<float>(row * SPRITE_SIZE),
        static_cast<float>(SPRITE_SIZE),
        static_cast<float>(SPRITE_SIZE)
    };

    float ts_f = static_cast<float>(tile_size);
    Rectangle dest_rect = {
        static_cast<float>(px),
        static_cast<float>(py),
        ts_f,
        ts_f
    };

    DrawTexturePro(tex, src_rect, dest_rect, { 0.0f, 0.0f }, 0.0f, RL_WHITE);
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

void Renderer::draw_frame(int px, int py, int w_tiles, int h_tiles) const
{
    if (!sheets_loaded) return;

    DrawRectangle(px, py, w_tiles * tile_size, h_tiles * tile_size, Color{ 8, 8, 16, 255 });

    int ts = tile_size;

    // Top border
    draw_tile_screen(px, py, GUI_FRAME_TL);
    for (int col = 1; col < w_tiles - 1; ++col)
    {
        draw_tile_screen(px + col * ts, py, GUI_FRAME_T);
    }
    draw_tile_screen(px + (w_tiles - 1) * ts, py, GUI_FRAME_TR);

    // Left and right borders
    for (int row = 1; row < h_tiles - 1; ++row)
    {
        draw_tile_screen(px,                       py + row * ts, GUI_FRAME_L);
        draw_tile_screen(px + (w_tiles - 1) * ts, py + row * ts, GUI_FRAME_R);
    }

    // Bottom border
    draw_tile_screen(px, py + (h_tiles - 1) * ts, GUI_FRAME_BL);
    for (int col = 1; col < w_tiles - 1; ++col)
    {
        draw_tile_screen(px + col * ts, py + (h_tiles - 1) * ts, GUI_FRAME_B);
    }
    draw_tile_screen(px + (w_tiles - 1) * ts, py + (h_tiles - 1) * ts, GUI_FRAME_BR);
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

void Renderer::set_camera_center(int world_tile_x, int world_tile_y, int map_w, int map_h)
{
    int map_viewport_rows = viewport_rows - GUI_RESERVE_ROWS;
    int viewport_px_w = viewport_cols * tile_size;
    int viewport_px_h = map_viewport_rows * tile_size;

    camera.x = world_tile_x * tile_size - viewport_px_w / 2;
    camera.y = world_tile_y * tile_size - viewport_px_h / 2;

    int map_px_w = map_w * tile_size;
    int map_px_h = map_h * tile_size;

    camera.x = std::clamp(camera.x, 0, std::max(0, map_px_w - viewport_px_w));
    camera.y = std::clamp(camera.y, 0, std::max(0, map_px_h - viewport_px_h));
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
        .map_cols = viewport_cols,
        .map_rows = viewport_rows - GUI_RESERVE_ROWS,
        .gui_rows = GUI_RESERVE_ROWS,
        .window_w = screen_w,
        .window_h = screen_h
    };
}

int Renderer::measure_text(std::string_view text) const
{
    std::string text_str(text);
    if (font_loaded)
    {
        Vector2 size = MeasureTextEx(game_font, text_str.c_str(), static_cast<float>(font_size), 1.0f);
        return static_cast<int>(size.x);
    }
    return MeasureText(text_str.c_str(), font_size);
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
