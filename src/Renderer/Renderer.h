#pragma once

#include <array>
#include <string>
#include <string_view>

#include <raylib.h>

#include "../Utils/Vector2D.h"

class TileConfig;

inline constexpr int SPRITE_SIZE = 16;       // DawnLike native sprite pixel size
inline constexpr int DISPLAY_TILE_SIZE = 64;  // Default rendered tile size in pixels
// The HUD's outer geometry, here rather than in Gui.cpp because two modules need
// it: the renderer reserves room for the HUD, and the Gui fills it.
//
// Its rows run on a pitch of their own rather than on the map's tile grid. A tile
// is as tall as the zoom makes it and a row of text is not, so a pitch measured in
// tiles puts most of a row on nothing at one zoom and overlaps at the next.
inline constexpr int GUI_TEXT_ROW_PITCH = 32;  // Pixels between the tops of two HUD text rows
inline constexpr int GUI_TEXT_TOP_INSET = 6;   // Gap below the frame's top edge before the first row
inline constexpr int GUI_TEXT_ROWS = 6;        // Text rows the HUD lays out

// Map-tile rows reserved at the bottom for the HUD.
//
// Derived rather than fixed. The HUD holds a constant amount of text, so the
// pixels it needs do not change with zoom; the number of tile rows covering them
// therefore must. As a constant it was 4, which is right only at a 64-pixel tile:
// at 16 the panel was 64 pixels tall for 198 pixels of content and the lower rows
// drew off the bottom of the screen, and at 96 it took 384 pixels to hold the same
// six rows.
//
// Example:
//
//   gui_reserve_rows(64, 16);   // -> 4, the value this replaced
//   gui_reserve_rows(16, 16);   // -> 13
//   gui_reserve_rows(96, 16);   // -> 3
[[nodiscard]] inline constexpr int gui_reserve_rows(int tileSize, int fontSize)
{
	// One whole tile for the frame's top edge, then the rows. The last row needs
	// only the height of the font rather than another full pitch.
	const int neededPixels =
		tileSize + GUI_TEXT_TOP_INSET + (GUI_TEXT_ROWS - 1) * GUI_TEXT_ROW_PITCH + fontSize;
	return (neededPixels + tileSize - 1) / tileSize;
}
inline constexpr int MAX_COLOR_PAIRS = 23;    // Size of the color pair table

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
	COUNT, // sentinel -- keep last
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
	int tile_w{};
	int tile_h{};
	int map_cols{};
	int map_rows{};
	int gui_rows{};
	int window_w{};
	int window_h{};
};

struct ColorPair
{
	Color fg{};
	Color bg{};
};

// Holds one DawnLike sprite sheet (optionally two frames for animation).
struct SpriteSheet
{
	Texture2D frame0{};
	Texture2D frame1{};
	int cellSize{ SPRITE_SIZE }; // source pixel size of one cell in this sheet
	int tilesPerRow{ 0 };
	int tilesPerCol{ 0 };
	bool animated{ false };
	bool loaded{ false };
	std::string_view name{};
};

class Renderer
{
	bool initialized{ false };

	int tileSize{ DISPLAY_TILE_SIZE };
	int viewportCols{ 0 };
	int viewportRows{ 0 };
	int screenWidth{ 0 };
	int screenHeight{ 0 };

	Vector2D camera{};

	float shakeTrauma{ 0.0f };
	Vector2D shakeOffset{};

	RenderTexture2D lightMask{};
	bool lightMaskLoaded{ false };

	std::array<SpriteSheet, static_cast<std::size_t>(TileSheet::COUNT)> sheets{};
	bool sheetsLoaded{ false };

	Font gameFont{};
	bool fontLoaded{ false };
	int fontSize{ 0 };

	int currentAnimFrame{ 0 };
	double lastAnimToggle{ 0.0 };

	std::array<ColorPair, MAX_COLOR_PAIRS> colorPairs{};

	void init_color_pairs();
	void load_sheet(TileSheet id, std::string_view name, std::string_view path0, std::string_view path1, int cellSize);
	void load_sheet_static(TileSheet id, std::string_view name, std::string_view path, int cellSize);

public:
	Renderer() = default;
	~Renderer() = default;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	void init();
	void shutdown();

	void load_dawnlike(std::string_view basePath);
	void load_font(std::string_view fontPath, int size);

	void begin_frame();
	void end_frame();

	// World-space tile drawing (camera offset applied)
	void draw_tile(Vector2D gridPos, TileRef tile, Color tint) const;
	// World-space tile drawing with a sub-tile pixel offset (e.g. open doors pushed to side)
	void draw_tile_offset(Vector2D gridPos, int pixelOffsetX, int pixelOffsetY, TileRef tile, Color tint) const;
	// World-space, always frame0 (no animation)
	void draw_tile_static(Vector2D gridPos, TileRef tile, Color tint) const;

	// Screen-space drawing (no camera offset)
	void draw_tile_screen(Vector2D screenPos, TileRef tile) const;
	void draw_tile_screen_color(Vector2D screenPos, TileRef tile, Color tint) const;
	void draw_tile_screen_color_sized(Vector2D screenPos, int size, TileRef tile, Color tint) const;

	// Screen-space drawing at an explicit pixel size (used by tile picker).
	void draw_tile_screen_sized(Vector2D screenPos, TileRef tile, int displaySize) const;
	void draw_text(Vector2D screenPos, std::string_view text, int colorPairId) const;
	void draw_text_color(Vector2D screenPos, std::string_view text, Color color) const;
	void draw_bar(Vector2D screenPos, int w, int h, float ratio, Color filled, Color empty) const;

	// Draw a DawnLike-tiled frame with dark background fill.
	// screenPos = top-left in pixels; wTiles/hTiles = dimensions in tiles.
	void draw_frame(Vector2D screenPos, int wTiles, int hTiles, const TileConfig& tileConfig) const;

	void set_camera_center(int world_tile_x, int world_tile_y, int map_w, int map_h);
	void update_viewport();
	void zoom_in();
	void zoom_out();

	// Screen shake: add trauma [0..1]. Decays automatically each frame.
	void add_trauma(float amount);

	// Dynamic lighting: fill darkness then punch light cones, multiply onto screen.
	void begin_light_mask();
	void add_light_quad(int screenX, int screenY, int tileSize, Color tileColor);
	void apply_light_mask();

	[[nodiscard]] ColorPair get_color_pair(int id) const;
	[[nodiscard]] ScreenMetrics metrics() const;
	[[nodiscard]] int measure_text(std::string_view text) const;
	// The longest prefix of text that fits in maxWidth pixels, cut at a space when
	// there is one so a word is not split across a boundary. Text that already fits
	// comes back unchanged, trailing space included, because that space separates it
	// from whatever is drawn beside it.
	//
	// Example, read off the HUD's log panel at 1280x896, where the panel measures
	// 575 pixels between its divider and the frame's right rule:
	//
	//   fit_text_to_width("You are now Well Fed.", 575);
	//   // -> "You are now Well Fed."          // fits, so it is returned whole
	//
	//   fit_text_to_width("mail, long sword, shield, long bow, fireball scroll.", 575);
	//   // -> "mail, long sword, shield, long" // cut at a space, never mid-word
	//
	// The cut lands on a space, so a caller that needs the tail of a string rather
	// than its head - an input field showing what was just typed - wants to limit
	// what it accepts instead of truncating what it draws.
	[[nodiscard]] std::string fit_text_to_width(std::string_view text, int maxWidth) const;

	[[nodiscard]] bool is_initialized() const { return initialized; }
	[[nodiscard]] int get_tile_size() const { return tileSize; }
	[[nodiscard]] int get_font_size() const { return fontSize; }
	// Tile rows the HUD needs at the current zoom. Everything that splits the
	// screen between map and HUD asks this rather than assuming a number.
	[[nodiscard]] int get_gui_reserve_rows() const { return gui_reserve_rows(tileSize, fontSize); }
	[[nodiscard]] int get_viewport_cols() const { return viewportCols; }
	[[nodiscard]] int get_viewport_rows() const { return viewportRows; }
	[[nodiscard]] int get_screen_width() const { return screenWidth; }
	[[nodiscard]] int get_screen_height() const { return screenHeight; }
	[[nodiscard]] int get_camera_x() const { return camera.x + shakeOffset.x; }
	[[nodiscard]] int get_camera_y() const { return camera.y + shakeOffset.y; }
	[[nodiscard]] int get_sheet_cols(TileSheet sheet) const;
	[[nodiscard]] int get_sheet_rows(TileSheet sheet) const;
	// Source pixel size of one cell in this sheet: 16 for DawnLike, 64 for the
	// regenerated sheets. Editors use it to draw tiles at an integer scale.
	[[nodiscard]] int get_sheet_cell_size(TileSheet sheet) const;
	[[nodiscard]] bool sheet_is_loaded(TileSheet sheet) const;
	[[nodiscard]] std::string_view get_sheet_name(TileSheet sheet) const;
	[[nodiscard]] int get_loaded_sheet_count() const;

};
