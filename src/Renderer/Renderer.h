#pragma once

#include <array>
#include <functional>
#include <string>
#include <unordered_map>

#include <raylib.h>

class TileConfig;

// DawnLike sprite sheet indices.
// Sheets with 0/1 suffixes are animation frame pairs.
enum class TileSheet
{
	SHEET_FLOOR,
	SHEET_WALL,
	SHEET_DOOR0,
	SHEET_DOOR1,
	SHEET_PLAYER0,
	SHEET_PLAYER1,
	SHEET_HUMANOID0,
	SHEET_HUMANOID1,
	SHEET_REPTILE0,
	SHEET_REPTILE1,
	SHEET_PEST0,
	SHEET_PEST1,
	SHEET_DOG0,
	SHEET_DOG1,
	SHEET_AVIAN0,
	SHEET_AVIAN1,
	SHEET_UNDEAD0,
	SHEET_UNDEAD1,
	SHEET_QUADRAPED0,
	SHEET_QUADRAPED1,
	SHEET_DEMON0,
	SHEET_DEMON1,
	SHEET_MISC0,
	SHEET_MISC1,
	SHEET_POTION,
	SHEET_SCROLL,
	SHEET_SHORT_WEP,
	SHEET_MED_WEP,
	SHEET_LONG_WEP,
	SHEET_ARMOR,
	SHEET_SHIELD,
	SHEET_HAT,
	SHEET_RING,
	SHEET_AMULET_ITEM,
	SHEET_FOOD,
	SHEET_FLESH,
	SHEET_MONEY,
	SHEET_TILE,
	SHEET_DECOR0,
	SHEET_DECOR1,
	SHEET_EFFECT0,
	SHEET_EFFECT1,
	SHEET_PIT0,
	SHEET_GUI0,
	SHEET_GUI1,
	// Items -- appended to preserve existing enum integer values
	SHEET_AMMO,
	SHEET_WAND,
	SHEET_BOOK,
	SHEET_BOOT,
	SHEET_GLOVE,
	SHEET_KEY,
	SHEET_LIGHT,
	SHEET_TOOL,
	SHEET_ROCK,
	SHEET_MUSIC,
	SHEET_CHEST0,
	// Characters -- appended to preserve existing enum integer values
	SHEET_SLIME0,
	SHEET_CAT0,
	SHEET_RODENT0,
	SHEET_PLANT0,
	SHEET_ELEMENTAL0,
	SHEET_AQUATIC0,
	// Objects -- appended to preserve existing enum integer values
	SHEET_ORE0,
	SHEET_HILL0,
	SHEET_TREE0,
	SHEET_GROUND0,
	SHEET_TRAP0,
	SHEET_FENCE,
	SHEET_MAP0,
};

template <>
struct std::hash<TileSheet>
{
	std::size_t operator()(TileSheet s) const noexcept
	{
		return std::hash<int>{}(static_cast<int>(s));
	}
};

// Unencoded tile reference: sheet + grid position.
// Default-constructed TileRef (col == -1) means "no tile".
struct TileRef
{
	TileSheet sheet{};
	int col{ -1 };
	int row{ -1 };

	[[nodiscard]] bool is_valid() const noexcept { return col >= 0; }
	[[nodiscard]] bool operator==(const TileRef& o) const noexcept
	{
		return sheet == o.sheet && col == o.col && row == o.row;
	}
};

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
inline constexpr int SPRITE_SIZE = 16; // DawnLike native sprite pixel size
inline constexpr int DISPLAY_TILE_SIZE = 32; // On-screen tile pixel size (2x)
inline constexpr int GUI_RESERVE_ROWS = 7;

// Holds one DawnLike sprite sheet (optionally two frames for animation).
struct SpriteSheet
{
	Texture2D frame0{};
	Texture2D frame1{};
	int tiles_per_row{ 0 };
	int tiles_per_col{ 0 };
	bool animated{ false };
	bool loaded{ false };
	std::string_view name;
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

	void load_dawnlike(std::string_view base_path);
	void load_font(std::string_view font_path, int size);

	void begin_frame();
	void end_frame();

	// World-space tile drawing (camera offset applied)
	void draw_tile(int grid_x, int grid_y, TileRef tile, int color_pair_id, Color tint) const;

	// Screen-space drawing (no camera offset)
	void draw_tile_screen(int px, int py, TileRef tile) const;

	// Screen-space drawing at an explicit pixel size (used by tile picker).
	void draw_tile_screen_sized(int px, int py, TileRef tile, int display_size) const;
	void draw_text(int px, int py, std::string_view text, int color_pair_id) const;
	void draw_text_color(int px, int py, std::string_view text, Color color) const;
	void draw_bar(int px, int py, int w, int h, float ratio, Color filled, Color empty) const;

	// Draw a DawnLike-tiled frame with dark background fill.
	// px/py = top-left in pixels; w_tiles/h_tiles = dimensions in tiles.
	void draw_frame(int px, int py, int w_tiles, int h_tiles, const TileConfig& tc) const;

	void set_camera_center(int world_tile_x, int world_tile_y, int map_w, int map_h);
	void update_viewport();
	void zoom_in();
	void zoom_out();

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
	[[nodiscard]] int get_sheet_cols(TileSheet sheet) const;
	[[nodiscard]] int get_sheet_rows(TileSheet sheet) const;
	[[nodiscard]] bool sheet_is_loaded(TileSheet sheet) const;
	[[nodiscard]] std::string_view get_sheet_name(TileSheet sheet) const;
	[[nodiscard]] int get_loaded_sheet_count() const;

private:
	void init_color_pairs();
	void load_sheet(TileSheet id, std::string_view name, const std::string& path0, const std::string& path1);
	void load_sheet_static(TileSheet id, std::string_view name, const std::string& path);

	bool initialized{ false };
	int tile_size{ DISPLAY_TILE_SIZE };
	int viewport_cols{ 0 };
	int viewport_rows{ 0 };
	int screen_w{ 0 };
	int screen_h{ 0 };

	GameCamera camera{};

	std::unordered_map<TileSheet, SpriteSheet> sheets;
	bool sheets_loaded{ false };

	Font game_font{};
	bool font_loaded{ false };
	int font_size{ 24 };

	int current_anim_frame{ 0 };
	double last_anim_toggle{ 0.0 };
	static constexpr double ANIM_INTERVAL = 0.5;

	std::array<ColorPair, MAX_COLOR_PAIRS> color_pairs{};
};
