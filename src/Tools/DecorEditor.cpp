// file: DecorEditor.cpp
#include <fstream>
#include <algorithm>
#include <format>

#pragma warning(push, 0)
#include <raylib.h>
#pragma warning(pop)

#include <nlohmann/json.hpp>

#include "DecorEditor.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/TileId.h"

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Sheet name table (indexed by TileSheet enum value)
// ---------------------------------------------------------------------------
static constexpr const char* SHEET_NAMES[SHEET_COUNT] =
{
    "Floor",      "Wall",       "Door0",       "Door1",
    "Player0",    "Player1",
    "Humanoid0",  "Humanoid1",
    "Reptile0",   "Reptile1",
    "Pest0",      "Pest1",
    "Dog0",       "Dog1",
    "Avian0",     "Avian1",
    "Undead0",    "Undead1",
    "Quadraped0", "Quadraped1",
    "Demon0",     "Demon1",
    "Misc0",      "Misc1",
    "Potion",     "Scroll",
    "ShortWeapon","MedWeapon",  "LongWeapon",
    "Armor",      "Shield",     "Hat",         "Ring",  "Amulet",
    "Food",       "Flesh",      "Money",
    "Tile",
    "Decor0",     "Decor1",
    "Effect0",    "Effect1",
    "Pit0",
    "GUI0",       "GUI1"
};

// ---------------------------------------------------------------------------

DecorEditor::DecorEditor() = default;

// ---------------------------------------------------------------------------
// Palette helpers
// ---------------------------------------------------------------------------

int DecorEditor::palette_find(int tile_id) const
{
    for (int i = 0; i < static_cast<int>(palette.size()); ++i)
    {
        if (palette[i].tile_id == tile_id)
            return i;
    }
    return -1;
}

void DecorEditor::palette_add_or_update(
    int             tile_id,
    std::string_view label,
    char            symbol
)
{
    int idx = palette_find(tile_id);
    if (idx >= 0)
    {
        palette[idx].label  = label;
        palette[idx].symbol = symbol;
    }
    else
    {
        palette.push_back({ tile_id, std::string(label), symbol });
    }
}

void DecorEditor::palette_remove(int tile_id)
{
    std::erase_if(palette, [tile_id](const PaletteEntry& e)
    {
        return e.tile_id == tile_id;
    });
    if (palette_index >= static_cast<int>(palette.size()))
        palette_index = std::max(0, static_cast<int>(palette.size()) - 1);
}

// ---------------------------------------------------------------------------
// Persistence -- palette
// ---------------------------------------------------------------------------

void DecorEditor::save_palette(std::string_view path) const
{
    json arr = json::array();
    for (const auto& e : palette)
    {
        json entry;
        entry["sheet"]  = tile_sheet(e.tile_id);
        entry["col"]    = tile_col(e.tile_id);
        entry["row"]    = tile_row(e.tile_id);
        entry["label"]  = e.label;
        entry["symbol"] = e.symbol ? std::string(1, e.symbol) : std::string{};
        arr.push_back(entry);
    }
    json j;
    j["tiles"] = arr;

    std::ofstream out(path.data());
    out << j.dump(2);
}

void DecorEditor::load_palette(std::string_view path)
{
    palette.clear();
    palette_index = 0;

    std::ifstream in(path.data());
    if (!in.is_open())
        return;

    json j;
    try
    {
        in >> j;
        for (const auto& e : j["tiles"])
        {
            PaletteEntry entry;
            int sheet   = e.value("sheet", 0);
            int col     = e.value("col",   0);
            int row     = e.value("row",   0);
            entry.tile_id = make_tile(sheet, col, row);
            entry.label   = e.value("label", "");
            std::string sym = e.value("symbol", "");
            entry.symbol = sym.empty() ? 0 : sym[0];
            if (!entry.label.empty())
                palette.push_back(std::move(entry));
        }
    }
    catch (...) {}
}

// ---------------------------------------------------------------------------
// Persistence -- decor overrides
// ---------------------------------------------------------------------------

void DecorEditor::save(std::string_view path) const
{
    json j = json::object();

    if (!active_key.empty())
        j["_last_saved"] = active_key;

    for (const auto& [map_key, tile_map] : all_overrides)
    {
        json arr = json::array();
        for (const auto& [coord_key, tile_id] : tile_map)
        {
            int x = static_cast<int>(coord_key & 0xFFFF);
            int y = static_cast<int>((coord_key >> 16) & 0xFFFF);
            arr.push_back({ {"x", x}, {"y", y}, {"tile", tile_id} });
        }
        j[map_key] = arr;
    }

    std::ofstream out(path.data());
    out << j.dump(2);
    last_save_time = GetTime();
}

void DecorEditor::load(std::string_view path)
{
    std::ifstream in(path.data());
    if (!in.is_open())
        return;

    json j;
    try
    {
        in >> j;
        all_overrides.clear();
        last_saved_key.clear();

        if (j.contains("_last_saved"))
            last_saved_key = j["_last_saved"].get<std::string>();

        for (auto& [map_key, arr] : j.items())
        {
            if (map_key.starts_with('_'))
                continue;

            auto& tile_map = all_overrides[map_key];
            for (const auto& entry : arr)
            {
                int x       = entry["x"].get<int>();
                int y       = entry["y"].get<int>();
                int tile_id = entry["tile"].get<int>();
                tile_map[make_key(x, y)] = tile_id;
            }
        }
    }
    catch (...) {}
}

// ---------------------------------------------------------------------------

void DecorEditor::set_active_map(long seed, int dungeon_level)
{
    active_key = std::format("{}_{}", seed, dungeon_level);
}

void DecorEditor::toggle()
{
    active = !active;
    if (!active)
    {
        browser_open = false;
        editing      = false;
    }
}

void DecorEditor::cycle_next()
{
    if (palette.empty()) return;
    palette_index = (palette_index + 1) % static_cast<int>(palette.size());
}

void DecorEditor::cycle_prev()
{
    if (palette.empty()) return;
    int sz = static_cast<int>(palette.size());
    palette_index = (palette_index - 1 + sz) % sz;
}

void DecorEditor::place(int world_x, int world_y)
{
    if (palette.empty()) return;
    current_map()[make_key(world_x, world_y)] = palette[palette_index].tile_id;
}

void DecorEditor::erase(int world_x, int world_y)
{
    current_map().erase(make_key(world_x, world_y));
}

void DecorEditor::place_tile(int world_x, int world_y, int tile_id)
{
    current_map()[make_key(world_x, world_y)] = tile_id;
}

bool DecorEditor::is_active_map_empty() const
{
    return current_map().empty();
}

int DecorEditor::get_override(int world_x, int world_y) const
{
    const auto& m = current_map();
    auto it = m.find(make_key(world_x, world_y));
    return (it != m.end()) ? it->second : 0;
}

std::unordered_map<uint32_t, int>& DecorEditor::current_map()
{
    return all_overrides[active_key];
}

const std::unordered_map<uint32_t, int>& DecorEditor::current_map() const
{
    auto it = all_overrides.find(active_key);
    if (it != all_overrides.end())
        return it->second;

    static const std::unordered_map<uint32_t, int> empty;
    return empty;
}

// ---------------------------------------------------------------------------
// update_and_render
// ---------------------------------------------------------------------------

void DecorEditor::update_and_render(const Renderer& renderer)
{
    if (!active)
        return;

    if (IsKeyPressed(KEY_TAB) && !editing)
    {
        browser_open = !browser_open;
        if (!browser_open)
            editing = false;
    }

    if (browser_open)
    {
        // Determine the palette path for auto-save (passed from caller via Paths)
        // We use a fixed path here matching what main.cpp uses.
        update_browser(renderer, "data/tile_labels.json");
        return;
    }

    // Normal editor overlay
    const int tile_size = renderer.get_tile_size();
    const int cam_x     = renderer.get_camera_x();
    const int cam_y     = renderer.get_camera_y();

    ::Vector2 mouse_pos = GetMousePosition();
    int world_x = (static_cast<int>(mouse_pos.x) + cam_x) / tile_size;
    int world_y = (static_cast<int>(mouse_pos.y) + cam_y) / tile_size;

    draw_cursor(renderer, world_x, world_y);
    draw_palette_strip(renderer);
    draw_info_bar(renderer, world_x, world_y);
}

// ---------------------------------------------------------------------------
// Normal editor drawing
// ---------------------------------------------------------------------------

void DecorEditor::draw_cursor(const Renderer& renderer, int world_x, int world_y) const
{
    if (palette.empty()) return;

    const int tile_size = renderer.get_tile_size();
    const int cam_x     = renderer.get_camera_x();
    const int cam_y     = renderer.get_camera_y();

    int px = world_x * tile_size - cam_x;
    int py = world_y * tile_size - cam_y;

    DrawRectangle(px, py, tile_size, tile_size, Color{ 255, 255, 0, 60 });
    DrawRectangleLines(px, py, tile_size, tile_size, Color{ 255, 255, 0, 220 });

    renderer.draw_tile(world_x, world_y, palette[palette_index].tile_id, 0,
        Color{ 255, 255, 255, 180 });
}

void DecorEditor::draw_palette_strip(const Renderer& renderer) const
{
    if (palette.empty()) return;

    const int tile_size  = renderer.get_tile_size();
    const int sw         = renderer.get_screen_width();
    const int sz         = static_cast<int>(palette.size());

    constexpr int VISIBLE = 9;
    int half = VISIBLE / 2;

    DrawRectangle(0, 0, sw, tile_size + 4, Color{ 0, 0, 0, 200 });

    for (int slot = 0; slot < VISIBLE; ++slot)
    {
        int idx = palette_index - half + slot;
        if (idx < 0 || idx >= sz)
            continue;

        int px = slot * tile_size + (sw - VISIBLE * tile_size) / 2;
        int py = 2;

        bool is_current = (idx == palette_index);

        if (is_current)
            DrawRectangle(px, py, tile_size, tile_size, Color{ 255, 255, 0, 80 });

        renderer.draw_tile_screen(px, py, palette[idx].tile_id);

        if (is_current)
            DrawRectangleLines(px, py, tile_size, tile_size, Color{ 255, 255, 0, 255 });
    }
}

void DecorEditor::draw_info_bar(const Renderer& renderer, int world_x, int world_y) const
{
    const int tile_size = renderer.get_tile_size();
    const int sw        = renderer.get_screen_width();

    int bar_y = tile_size + 6;
    DrawRectangle(0, bar_y, sw, 20, Color{ 0, 0, 0, 180 });

    bool saved_flash = (GetTime() - last_save_time) < 2.0;

    std::string tile_name = palette.empty()
        ? "(no tiles -- Tab to open browser)"
        : palette[palette_index].label;

    std::string info = std::format(
        "DECOR EDITOR  |  {}  |  Map: {}  |  Pos: {},{}"
        "  |  Tab browser  , / . cycle  LClick place  RClick erase  Ctrl+S save{}",
        tile_name,
        active_key.empty() ? "none" : active_key,
        world_x, world_y,
        saved_flash ? "  -- SAVED!" : ""
    );

    Color text_color = saved_flash
        ? Color{ 100, 255, 100, 255 }
        : Color{ 255, 255, 180, 255 };

    renderer.draw_text_color(4, bar_y + 2, info, text_color);
}

// ---------------------------------------------------------------------------
// Sheet browser
// ---------------------------------------------------------------------------

void DecorEditor::update_browser(const Renderer& renderer, std::string_view palette_path)
{
    const int sw = renderer.get_screen_width();
    const int sh = renderer.get_screen_height();

    constexpr int BROWSER_TILE = 32;
    constexpr int PAD          = 8;
    constexpr int HEADER_H     = 32;
    constexpr int EDIT_H       = 88;

    // Full-screen dark backdrop
    DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 220 });

    // --- Sheet navigation (Left/Right arrow) ---
    auto advance_sheet = [&](int dir)
    {
        for (int i = 0; i < SHEET_COUNT; ++i)
        {
            browser_sheet = (browser_sheet + dir + SHEET_COUNT) % SHEET_COUNT;
            if (renderer.sheet_is_loaded(browser_sheet))
                break;
        }
        browser_scroll   = 0;
        browser_selected = -1;
        editing          = false;
    };

    if (IsKeyPressed(KEY_LEFT))  advance_sheet(-1);
    if (IsKeyPressed(KEY_RIGHT)) advance_sheet( 1);

    // Ensure we start on a loaded sheet
    if (!renderer.sheet_is_loaded(browser_sheet))
        advance_sheet(1);

    const int sheet_cols = renderer.get_sheet_cols(browser_sheet);
    const int sheet_rows = renderer.get_sheet_rows(browser_sheet);

    // --- Header ---
    DrawRectangle(0, 0, sw, HEADER_H, Color{ 20, 20, 40, 255 });
    std::string header = std::format(
        "TILE BROWSER  |  {} ({}/{})  |  {}x{} tiles"
        "  |  <- / -> sheet  |  Wheel scroll  |  Tab close",
        SHEET_NAMES[browser_sheet],
        browser_sheet + 1, static_cast<int>(SHEET_COUNT),
        sheet_cols, sheet_rows
    );
    renderer.draw_text_color(PAD, 8, header, Color{ 255, 255, 180, 255 });

    // --- Tile grid area ---
    const int grid_y    = HEADER_H + 2;
    const int grid_h    = sh - grid_y - (editing ? EDIT_H : 0) - 2;

    // Scroll via mouse wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f)
    {
        browser_scroll -= static_cast<int>(wheel);
        browser_scroll = std::max(0, std::min(browser_scroll, sheet_rows - 1));
    }

    ::Vector2 mouse  = GetMousePosition();
    bool      clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    BeginScissorMode(0, grid_y, sw, grid_h);

    for (int row = browser_scroll; row < sheet_rows; ++row)
    {
        int py = grid_y + (row - browser_scroll) * (BROWSER_TILE + 2);
        if (py >= grid_y + grid_h)
            break;

        for (int col = 0; col < sheet_cols; ++col)
        {
            int px  = PAD + col * (BROWSER_TILE + 2);
            int tid = make_tile(browser_sheet, col, row);

            bool is_sel  = (tid == browser_selected);
            bool in_pal  = (palette_find(tid) >= 0);
            bool hovered = (mouse.x >= px && mouse.x < px + BROWSER_TILE
                         && mouse.y >= py && mouse.y < py + BROWSER_TILE);

            if (in_pal)
                DrawRectangle(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 20, 60, 20, 200 });
            if (is_sel)
                DrawRectangle(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 60, 60, 0, 220 });
            if (hovered && !is_sel)
                DrawRectangle(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 60, 60, 30, 160 });

            renderer.draw_tile_screen_sized(px, py, tid, BROWSER_TILE);

            if (is_sel)
                DrawRectangleLines(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 255, 255, 0, 255 });
            else if (in_pal)
                DrawRectangleLines(px, py, BROWSER_TILE, BROWSER_TILE, Color{ 60, 220, 60, 200 });

            // Tooltip on hover
            if (hovered)
            {
                int pi = palette_find(tid);
                if (pi >= 0)
                {
                    const auto& pe = palette[pi];
                    std::string tip = pe.label;
                    if (pe.symbol)
                        tip += std::format(" [{}]", pe.symbol);
                    int tip_x = static_cast<int>(mouse.x) + 14;
                    int tip_y = static_cast<int>(mouse.y) - 18;
                    DrawRectangle(tip_x - 2, tip_y - 2,
                        static_cast<int>(tip.size()) * 8 + 6, 18,
                        Color{ 0, 0, 0, 200 });
                    renderer.draw_text_color(tip_x, tip_y, tip, Color{ 255, 255, 200, 255 });
                }
            }

            // Click to select
            if (hovered && clicked)
            {
                browser_selected = tid;
                int pi = palette_find(tid);
                if (pi >= 0)
                {
                    label_buf   = palette[pi].label;
                    sym_buf     = palette[pi].symbol;
                }
                else
                {
                    label_buf = std::format("{} {},{}", SHEET_NAMES[browser_sheet], col, row);
                    sym_buf   = 0;
                }
                editing     = true;
                editing_sym = false;
            }
        }
    }

    EndScissorMode();

    // Delete key removes selected tile from palette
    if (browser_selected >= 0 && IsKeyPressed(KEY_DELETE))
    {
        palette_remove(browser_selected);
        save_palette(palette_path);
        editing          = false;
        browser_selected = -1;
    }

    // --- Edit panel ---
    if (editing)
    {
        int panel_y = sh - EDIT_H;
        DrawRectangle(0, panel_y, sw, EDIT_H, Color{ 10, 10, 30, 248 });
        DrawLine(0, panel_y, sw, panel_y, Color{ 100, 100, 200, 255 });

        // Show selected tile preview
        renderer.draw_tile_screen_sized(PAD, panel_y + (EDIT_H - BROWSER_TILE) / 2,
            browser_selected, BROWSER_TILE);

        int text_x = PAD + BROWSER_TILE + 12;

        // Label field
        Color lbl_col = !editing_sym
            ? Color{ 255, 255, 100, 255 }
            : Color{ 160, 160,  80, 255 };
        std::string lbl_text = std::format("Label: {}{}",
            label_buf, !editing_sym ? "|" : "");
        renderer.draw_text_color(text_x, panel_y + 8, lbl_text, lbl_col);

        // Symbol field
        Color sym_col = editing_sym
            ? Color{ 255, 255, 100, 255 }
            : Color{ 160, 160,  80, 255 };
        std::string sym_text = std::format("Symbol (prefab char): {}{}",
            sym_buf ? std::string(1, sym_buf) : "_",
            editing_sym ? "|" : "");
        renderer.draw_text_color(text_x, panel_y + 28, sym_text, sym_col);

        renderer.draw_text_color(text_x, panel_y + 56,
            "Tab = switch field  |  Enter = save  |  Escape = cancel  |  Delete = remove",
            Color{ 140, 140, 140, 255 });

        // Switch fields
        if (IsKeyPressed(KEY_TAB))
        {
            editing_sym = !editing_sym;
            // Swallow -- do NOT close browser
        }

        if (IsKeyPressed(KEY_ESCAPE))
        {
            editing          = false;
            browser_selected = -1;
        }

        if (IsKeyPressed(KEY_ENTER) && !label_buf.empty())
        {
            palette_add_or_update(browser_selected, label_buf, sym_buf);
            save_palette(palette_path);
            editing = false;
        }

        if (!editing_sym)
        {
            if (IsKeyPressed(KEY_BACKSPACE) && !label_buf.empty())
                label_buf.pop_back();

            int ch = GetCharPressed();
            while (ch > 0)
            {
                if (ch >= 32 && ch < 127)
                    label_buf += static_cast<char>(ch);
                ch = GetCharPressed();
            }
        }
        else
        {
            // Single char symbol
            if (IsKeyPressed(KEY_BACKSPACE))
                sym_buf = 0;

            int ch = GetCharPressed();
            if (ch > 32 && ch < 127)
                sym_buf = static_cast<char>(ch);
        }
    }
}
