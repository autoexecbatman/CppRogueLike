#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include <raylib.h>

#include "TileId.h"

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
	void draw_tile(int grid_x, int grid_y, int tile_id, int color_pair_id, Color tint) const;

	// Screen-space drawing (no camera offset)
	void draw_tile_screen(int px, int py, int tile_id) const;

	// Screen-space drawing at an explicit pixel size (used by tile picker).
	void draw_tile_screen_sized(int px, int py, int tile_id, int display_size) const;
	void draw_text(int px, int py, std::string_view text, int color_pair_id) const;
	void draw_text_color(int px, int py, std::string_view text, Color color) const;
	void draw_bar(int px, int py, int w, int h, float ratio, Color filled, Color empty) const;

	// Draw a DawnLike-tiled frame with dark background fill.
	// px/py = top-left in pixels; w_tiles/h_tiles = dimensions in tiles.
	void draw_frame(int px, int py, int w_tiles, int h_tiles) const;

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
