// file: Renderer.cpp
#include <algorithm>
#include <format>
#include <ranges>
#include <string>

#include <raylib.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <rlgl.h>
#endif

#include "../Systems/TileConfig.h"
#include "Renderer.h"

// Local color constants (raylib macros are #undef'd in Renderer.h)
static constexpr Color RL_WHITE = { 255, 255, 255, 255 };
static constexpr Color RL_BLACK = { 0, 0, 0, 255 };
static constexpr Color RL_RED = { 230, 41, 55, 255 };
static constexpr Color RL_GREEN = { 0, 228, 48, 255 };
static constexpr Color RL_BLUE = { 0, 121, 241, 255 };
static constexpr Color RL_YELLOW = { 253, 249, 0, 255 };
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
	// Query the actual browser window size and snap to tile grid so no partial
	// tiles appear at the edges.
	{
		int raw_w = EM_ASM_INT({ return window.innerWidth; });
		int raw_h = EM_ASM_INT({ return window.innerHeight; });
		screen_w = (raw_w / tile_size) * tile_size;
		screen_h = (raw_h / tile_size) * tile_size;
	}
	InitWindow(screen_w, screen_h, "C++RogueLike");
	// Browser rAF drives timing -- disable raylib's internal WaitTime/emscripten_sleep.
	SetTargetFPS(0);
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

void Renderer::update_viewport()
{
	screen_w = GetScreenWidth();
	screen_h = GetScreenHeight();
	viewport_cols = screen_w / tile_size;
	viewport_rows = screen_h / tile_size;
}

void Renderer::shutdown()
{
	for (SpriteSheet& sheet : sheets | std::views::values)
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

void Renderer::load_sheet(TileSheet id, std::string_view name, const std::string& path0, const std::string& path1)
{
	auto& s = sheets[id];
	s.name = name;
	s.frame0 = load_dawnlike_texture(path0.c_str());
	s.frame1 = load_dawnlike_texture(path1.c_str());
	s.tiles_per_row = s.frame0.width / SPRITE_SIZE;
	s.tiles_per_col = s.frame0.height / SPRITE_SIZE;
	s.animated = (s.frame1.id > 0);
	s.loaded = (s.frame0.id > 0);
}

void Renderer::load_sheet_static(TileSheet id, std::string_view name, const std::string& path)
{
	auto& s = sheets[id];
	s.name = name;
	s.frame0 = load_dawnlike_texture(path.c_str());
	s.frame1 = s.frame0;
	s.tiles_per_row = s.frame0.width / SPRITE_SIZE;
	s.tiles_per_col = s.frame0.height / SPRITE_SIZE;
	s.animated = false;
	s.loaded = (s.frame0.id > 0);
}

int Renderer::get_sheet_cols(TileSheet sheet) const
{
	if (!sheets.contains(sheet))
		return 0;
	return sheets.at(sheet).tiles_per_row;
}

int Renderer::get_sheet_rows(TileSheet sheet) const
{
	if (!sheets.contains(sheet))
		return 0;
	return sheets.at(sheet).tiles_per_col;
}

bool Renderer::sheet_is_loaded(TileSheet sheet) const
{
	if (!sheets.contains(sheet))
		return false;
	return sheets.at(sheet).loaded;
}

std::string_view Renderer::get_sheet_name(TileSheet sheet) const
{
	if (!sheets.contains(sheet))
		return {};
	return sheets.at(sheet).name;
}

int Renderer::get_loaded_sheet_count() const
{
	return static_cast<int>(sheets.size());
}

void Renderer::load_dawnlike(std::string_view base_path)
{
	std::string base(base_path);
	if (!base.empty() && base.back() != '/')
	{
		base += '/';
	}

	auto load_animated = [&](TileSheet id, std::string_view name, const char* dir, const char* file)
	{
		std::string p0 = std::format("{}{}{}0.png", base, dir, file);
		std::string p1 = std::format("{}{}{}1.png", base, dir, file);
		load_sheet(id, name, p0, p1);
	};

	auto load_static = [&](TileSheet id, std::string_view name, const char* dir, const char* file)
	{
		std::string p = std::format("{}{}{}.png", base, dir, file);
		load_sheet_static(id, name, p);
	};

	// Objects
	load_static(TileSheet::SHEET_FLOOR, "Floor", "Objects/", "Floor");
	load_static(TileSheet::SHEET_WALL, "Wall", "Objects/", "Wall");
	load_static(TileSheet::SHEET_DOOR0, "Door0", "Objects/", "Door0");
	load_animated(TileSheet::SHEET_DECOR0, "Decor0", "Objects/", "Decor");
	load_animated(TileSheet::SHEET_EFFECT0, "Effect0", "Objects/", "Effect");
	load_static(TileSheet::SHEET_TILE, "Tile", "Objects/", "Tile");
	load_animated(TileSheet::SHEET_PIT0, "Pit0", "Objects/", "Pit");
	load_animated(TileSheet::SHEET_GUI0, "GUI0", "GUI/", "GUI");

	// Characters (all animated with 0/1 pairs)
	load_animated(TileSheet::SHEET_PLAYER0, "Player0", "Characters/", "Player");
	load_animated(TileSheet::SHEET_HUMANOID0, "Humanoid0", "Characters/", "Humanoid");
	load_animated(TileSheet::SHEET_REPTILE0, "Reptile0", "Characters/", "Reptile");
	load_animated(TileSheet::SHEET_PEST0, "Pest0", "Characters/", "Pest");
	load_animated(TileSheet::SHEET_DOG0, "Dog0", "Characters/", "Dog");
	load_animated(TileSheet::SHEET_AVIAN0, "Avian0", "Characters/", "Avian");
	load_animated(TileSheet::SHEET_UNDEAD0, "Undead0", "Characters/", "Undead");
	load_animated(TileSheet::SHEET_QUADRAPED0, "Quadraped0", "Characters/", "Quadraped");
	load_animated(TileSheet::SHEET_DEMON0, "Demon0", "Characters/", "Demon");
	load_animated(TileSheet::SHEET_MISC0, "Misc0", "Characters/", "Misc");

	// Items (static -- no animation frames)
	load_static(TileSheet::SHEET_POTION, "Potion", "Items/", "Potion");
	load_static(TileSheet::SHEET_SCROLL, "Scroll", "Items/", "Scroll");
	load_static(TileSheet::SHEET_SHORT_WEP, "ShortWep", "Items/", "ShortWep");
	load_static(TileSheet::SHEET_MED_WEP, "MedWep", "Items/", "MedWep");
	load_static(TileSheet::SHEET_LONG_WEP, "LongWep", "Items/", "LongWep");
	load_static(TileSheet::SHEET_ARMOR, "Armor", "Items/", "Armor");
	load_static(TileSheet::SHEET_SHIELD, "Shield", "Items/", "Shield");
	load_static(TileSheet::SHEET_HAT, "Hat", "Items/", "Hat");
	load_static(TileSheet::SHEET_RING, "Ring", "Items/", "Ring");
	load_static(TileSheet::SHEET_AMULET_ITEM, "Amulet", "Items/", "Amulet");
	load_static(TileSheet::SHEET_FOOD, "Food", "Items/", "Food");
	load_static(TileSheet::SHEET_FLESH, "Flesh", "Items/", "Flesh");
	load_static(TileSheet::SHEET_MONEY, "Money", "Items/", "Money");

	// Previously unloaded item sheets
	load_static(TileSheet::SHEET_AMMO, "Ammo", "Items/", "Ammo");
	load_static(TileSheet::SHEET_WAND, "Wand", "Items/", "Wand");
	load_static(TileSheet::SHEET_BOOK, "Book", "Items/", "Book");
	load_static(TileSheet::SHEET_BOOT, "Boot", "Items/", "Boot");
	load_static(TileSheet::SHEET_GLOVE, "Glove", "Items/", "Glove");
	load_static(TileSheet::SHEET_KEY, "Key", "Items/", "Key");
	load_static(TileSheet::SHEET_LIGHT, "Light", "Items/", "Light");
	load_static(TileSheet::SHEET_TOOL, "Tool", "Items/", "Tool");
	load_static(TileSheet::SHEET_ROCK, "Rock", "Items/", "Rock");
	load_static(TileSheet::SHEET_MUSIC, "Music", "Items/", "Music");
	load_animated(TileSheet::SHEET_CHEST0, "Chest0", "Items/", "Chest");

	// Previously unloaded character sheets
	load_animated(TileSheet::SHEET_SLIME0, "Slime0", "Characters/", "Slime");
	load_animated(TileSheet::SHEET_CAT0, "Cat0", "Characters/", "Cat");
	load_animated(TileSheet::SHEET_RODENT0, "Rodent0", "Characters/", "Rodent");
	load_animated(TileSheet::SHEET_PLANT0, "Plant0", "Characters/", "Plant");
	load_animated(TileSheet::SHEET_ELEMENTAL0, "Elemental0", "Characters/", "Elemental");
	load_animated(TileSheet::SHEET_AQUATIC0, "Aquatic0", "Characters/", "Aquatic");

	// Previously unloaded object sheets
	load_animated(TileSheet::SHEET_ORE0, "Ore0", "Objects/", "Ore");
	load_animated(TileSheet::SHEET_HILL0, "Hill0", "Objects/", "Hill");
	load_animated(TileSheet::SHEET_TREE0, "Tree0", "Objects/", "Tree");
	load_animated(TileSheet::SHEET_GROUND0, "Ground0", "Objects/", "Ground");
	load_animated(TileSheet::SHEET_TRAP0, "Trap0", "Objects/", "Trap");
	load_static(TileSheet::SHEET_FENCE, "Fence", "Objects/", "Fence");
	load_animated(TileSheet::SHEET_MAP0, "Map0", "Objects/", "Map");

	sheets_loaded = true;
}

void Renderer::load_font(std::string_view font_path, int size)
{
	font_size = size;
	game_font = LoadFontEx(font_path.data(), size, nullptr, 256);
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
#ifdef EMSCRIPTEN
	// EndDrawing's order is: flush -> swap -> WaitTime -> PollInputEvents.
	// glfwSwapBuffers may yield (emscripten_sleep), so events can fire between
	// swap and PollInputEvents, causing PollInputEvents to see prev=curr=1 and
	// making IsKeyPressed permanently false.
	// Fix: flush -> PollInputEvents -> swap.
	// Events that arrive during the swap yield are captured next frame with prev=0.
	rlDrawRenderBatchActive();
	PollInputEvents();
	SwapScreenBuffer();
#else
	EndDrawing();
#endif
}

void Renderer::draw_tile(int grid_x, int grid_y, TileRef tile, int /*color_pair_id*/, Color tint) const
{
	if (!sheets_loaded || !tile.is_valid())
	{
		return;
	}

	if (!sheets.contains(tile.sheet))
	{
		return;
	}
	const SpriteSheet& sheet = sheets.at(tile.sheet);
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

	// Source rect in the 16x16 sprite sheet
	Rectangle src_rect = {
		static_cast<float>(tile.col * SPRITE_SIZE),
		static_cast<float>(tile.row * SPRITE_SIZE),
		static_cast<float>(SPRITE_SIZE),
		static_cast<float>(SPRITE_SIZE)
	};

	// Destination rect scaled to display tile size
	Rectangle dest_rect = { dest_x, dest_y, ts_f, ts_f };

	DrawTexturePro(tex, src_rect, dest_rect, { 0.0f, 0.0f }, 0.0f, tint);
}

void Renderer::draw_tile_screen(int px, int py, TileRef tile) const
{
	if (!sheets_loaded || !tile.is_valid())
	{
		return;
	}

	if (!sheets.contains(tile.sheet))
	{
		return;
	}
	const SpriteSheet& sheet = sheets.at(tile.sheet);
	if (!sheet.loaded || sheet.tiles_per_row <= 0)
	{
		return;
	}

	const Texture2D& tex = (sheet.animated && current_anim_frame == 1)
		? sheet.frame1
		: sheet.frame0;

	Rectangle src_rect = {
		static_cast<float>(tile.col * SPRITE_SIZE),
		static_cast<float>(tile.row * SPRITE_SIZE),
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

void Renderer::draw_tile_screen_sized(int px, int py, TileRef tile, int display_size) const
{
	if (!sheets_loaded || !tile.is_valid())
		return;

	if (!sheets.contains(tile.sheet))
		return;
	const SpriteSheet& sheet = sheets.at(tile.sheet);
	if (!sheet.loaded || sheet.tiles_per_row <= 0)
		return;

	const Texture2D& tex = (sheet.animated && current_anim_frame == 1)
		? sheet.frame1
		: sheet.frame0;

	Rectangle src_rect = {
		static_cast<float>(tile.col * SPRITE_SIZE),
		static_cast<float>(tile.row * SPRITE_SIZE),
		static_cast<float>(SPRITE_SIZE),
		static_cast<float>(SPRITE_SIZE)
	};

	float ds_f = static_cast<float>(display_size);
	Rectangle dest_rect = {
		static_cast<float>(px),
		static_cast<float>(py),
		ds_f,
		ds_f
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

void Renderer::draw_text_color(int px, int py, std::string_view text, Color color) const
{
	std::string text_str(text);

	if (font_loaded)
	{
		Vector2 pos = { static_cast<float>(px), static_cast<float>(py) };
		DrawTextEx(game_font, text_str.c_str(), pos, static_cast<float>(font_size), 1.0f, color);
	}
	else
	{
		DrawText(text_str.c_str(), px, py, font_size, color);
	}
}

void Renderer::zoom_in()
{
	static constexpr int zoom_levels[] = { 16, 24, 32, 48 };
	for (int i = 0; i < 3; ++i)
	{
		if (tile_size == zoom_levels[i])
		{
			tile_size = zoom_levels[i + 1];
			font_size = tile_size * 3 / 4;
			update_viewport();
			return;
		}
	}
}

void Renderer::zoom_out()
{
	static constexpr int zoom_levels[] = { 16, 24, 32, 48 };
	for (int i = 1; i < 4; ++i)
	{
		if (tile_size == zoom_levels[i])
		{
			tile_size = zoom_levels[i - 1];
			font_size = tile_size * 3 / 4;
			update_viewport();
			return;
		}
	}
}

void Renderer::draw_frame(int px, int py, int w_tiles, int h_tiles, const TileConfig& tc) const
{
	if (!sheets_loaded)
		return;

	DrawRectangle(px, py, w_tiles * tile_size, h_tiles * tile_size, Color{ 8, 8, 16, 255 });

	int ts = tile_size;

	// Top border
	draw_tile_screen(px, py, tc.get("GUI_FRAME_TL"));
	for (int col = 1; col < w_tiles - 1; ++col)
	{
		draw_tile_screen(px + col * ts, py, tc.get("GUI_FRAME_T"));
	}
	draw_tile_screen(px + (w_tiles - 1) * ts, py, tc.get("GUI_FRAME_TR"));

	// Left and right borders
	for (int row = 1; row < h_tiles - 1; ++row)
	{
		draw_tile_screen(px, py + row * ts, tc.get("GUI_FRAME_L"));
		draw_tile_screen(px + (w_tiles - 1) * ts, py + row * ts, tc.get("GUI_FRAME_R"));
	}

	// Bottom border
	draw_tile_screen(px, py + (h_tiles - 1) * ts, tc.get("GUI_FRAME_BL"));
	for (int col = 1; col < w_tiles - 1; ++col)
	{
		draw_tile_screen(px + col * ts, py + (h_tiles - 1) * ts, tc.get("GUI_FRAME_B"));
	}
	draw_tile_screen(px + (w_tiles - 1) * ts, py + (h_tiles - 1) * ts, tc.get("GUI_FRAME_BR"));
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
	color_pairs[4] = ColorPair{ RL_WHITE, Color{ 0, 102, 0, 255 } }; // dim green bg

	// === BLACK FOREGROUND PAIRS ===
	color_pairs[5] = ColorPair{ RL_BLACK, RL_WHITE };
	color_pairs[6] = ColorPair{ RL_BLACK, RL_GREEN };
	color_pairs[7] = ColorPair{ RL_BLACK, RL_YELLOW };
	color_pairs[8] = ColorPair{ RL_BLACK, RL_RED };

	// === COLORED FOREGROUND ON BLACK ===
	color_pairs[9] = ColorPair{ RL_RED, RL_BLACK };
	color_pairs[10] = ColorPair{ RL_GREEN, RL_BLACK };
	color_pairs[11] = ColorPair{ RL_YELLOW, RL_BLACK };
	color_pairs[12] = ColorPair{ RL_BLUE, RL_BLACK };
	color_pairs[13] = ColorPair{ Color{ 0, 255, 255, 255 }, RL_BLACK }; // cyan
	color_pairs[14] = ColorPair{ RL_MAGENTA, RL_BLACK };

	// === SPECIAL COMBINATIONS ===
	color_pairs[15] = ColorPair{ Color{ 0, 255, 255, 255 }, RL_BLUE }; // cyan on blue
	color_pairs[16] = ColorPair{ RL_RED, RL_WHITE };
	color_pairs[17] = ColorPair{ RL_GREEN, RL_YELLOW };
	color_pairs[18] = ColorPair{ RL_GREEN, RL_MAGENTA };
	color_pairs[19] = ColorPair{ RL_RED, RL_YELLOW };
	color_pairs[20] = ColorPair{ RL_GREEN, RL_RED };

	// === CUSTOM COLORS ===
	color_pairs[21] = ColorPair{ Color{ 128, 77, 0, 255 }, RL_BLACK }; // brown
	color_pairs[22] = ColorPair{ Color{ 0, 102, 0, 255 }, RL_BLACK }; // dim green
}

// end of file: Renderer.cpp
