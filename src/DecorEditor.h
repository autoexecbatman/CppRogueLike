// file: DecorEditor.h
#pragma once

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Renderer.h"
#include "Vector2D.h"

// ---------------------------------------------------------------------------
// DecorEditor -- in-game decoration placement and tile labeling tool.
//
// F2           toggle editor mode
// Tab          toggle sprite sheet browser
// , / .        cycle palette tile
// Left click   place current palette tile at hovered world tile
// Right click  erase decoration at hovered world tile
// Ctrl+S       save overrides + palette
//
// Browser mode (Tab):
//   Left/Right arrows   cycle sprite sheets
//   Wheel (in grid)     scroll tile grid
//   Wheel (in list)     scroll palette list
//   Click list entry    select entry -- jumps to its sheet, enters reassign mode
//   Click tile (normal) select tile for new label / symbol
//   Click tile (reassign) reassign sprite of the selected palette entry
//   Tab                 switch between label and symbol fields
//   Enter               save label / symbol (and sprite if reassigned)
//   Escape              cancel -- clear selection
//   Delete              remove selected tile from palette
// ---------------------------------------------------------------------------
class DecorEditor
{
private:
	struct PaletteEntry
	{
		TileRef tile{};
		std::string label{};
		char symbol{ 0 }; // 0 = no symbol (tile not usable in prefabs)
	};

	// nlohmann's readers and writers for a palette row, found by argument-dependent lookup.
	friend void to_json(nlohmann::json& output, const PaletteEntry& entry);
	friend void from_json(const nlohmann::json& source, PaletteEntry& entry);

	bool active{ false };
	size_t paletteIndex{ 0 };
	double lastSaveTime{ -10.0 };
	std::optional<std::string> saveError{}; // why the last reported save was refused, until one succeeds

	std::string activeKey{};
	std::string palettePath{}; // the file load_palette read, where save_palette_reporting writes

	std::unordered_map<std::string, std::unordered_map<uint32_t, TileRef>> allOverrides{};

	// Dynamic palette -- loaded from tile_labels.json, editable at runtime.
	std::vector<PaletteEntry> palette{};

	// Sheet browser state
	bool isBrowserOpen{ false };
	int browserSheet{ 0 };
	int browserScroll{ 0 };
	TileRef browserSelected{}; // selected tile, is_valid() == false means none

	// Palette list panel state
	std::optional<size_t> listCursor{}; // the selected palette entry, if one is chosen
	size_t listScroll{ 0 };

	// Label / symbol editing state
	bool editing{ false };
	std::string labelBuffer{};
	char symbolBuffer{ 0 };
	bool isEditingSymbol{ false }; // true = cursor in symbol field
	bool isLabelAllSelected{ false }; // first keystroke replaces entire label
	int bufferedCharacter{ 0 }; // char polled by InputSystem::poll(), fed in before render

	// Packs a tile position into one key: row in the high half, column in the
	// low half. Columns wrap at 65536, which no map approaches.
	[[nodiscard]] static uint32_t make_key(Vector2D worldPosition) noexcept
	{
		return (static_cast<uint32_t>(worldPosition.y) << 16) | static_cast<uint32_t>(worldPosition.x & 0xFFFF);
	}

	[[nodiscard]] std::unordered_map<uint32_t, TileRef>& current_map();
	[[nodiscard]] const std::unordered_map<uint32_t, TileRef>& current_map() const;

	// Normal editor overlay
	void draw_cursor(const Renderer& renderer, Vector2D worldPosition) const;
	void draw_palette_strip(const Renderer& renderer) const;
	void draw_info_bar(const Renderer& renderer, Vector2D worldPosition) const;

	// Sheet browser
	void update_browser(const Renderer& renderer);

	// Palette helpers
	[[nodiscard]] std::optional<size_t> palette_find(TileRef tile) const;
	void palette_add_or_update(TileRef tile, std::string_view label, char symbol);
	void palette_remove(TileRef tile);

public:
	DecorEditor();

	void toggle();

	void set_char_input(int character) noexcept { bufferedCharacter = character; }

	void cycle_next();
	void cycle_prev();

	void set_active_map(long seed, int dungeonLevel);

	// Writes the palette into the "palette" section of the file at `path`, keeping every
	// other section. Throws std::runtime_error naming the path when the file does not open
	// or parse, which would lose those sections, and leaves it untouched. Also throws when
	// the write fails, which may leave the file partly written.
	//
	// Example:
	//   editor.save_palette(Paths::TILE_CONFIG);   // the palette section now holds the editor's entries
	//   editor.save_palette("data/missing.json");  // throws: "... cannot open '...missing.json' to keep its other sections"
	void save_palette(std::string_view path) const;

	// Reads the "palette" section of the file at `path`, keeping only labelled entries.
	// Every entry needs sheet, col, row, label and symbol. Throws when the file does not
	// open or parse, has no "palette", or an entry lacks a field; the palette loaded before
	// is then kept.
	//
	// Example:
	//   editor.load_palette(Paths::TILE_CONFIG);   // every labelled entry of the shipped palette
	//   editor.load_palette("data/missing.json");  // throws: "... cannot open '...missing.json' -- the palette is required"
	void load_palette(std::string_view path);

	// Saves to the file the palette was loaded from, for the editor's own save keys. A
	// refusal is kept for save_error() and drawn until a later save succeeds.
	//
	// Example:
	//   editor.save_palette_reporting();   // saved: save_error() -> nullopt, "SAVED!" shows for two seconds
	//   editor.save_palette_reporting();   // file unparseable: save_error() -> "... '...tile_config.json' does not parse: ..."
	void save_palette_reporting();

	// Why the last reported save was refused, or nothing once one succeeds.
	[[nodiscard]] const std::optional<std::string>& save_error() const noexcept { return saveError; }

	// Called every render frame while active.
	void update_and_render(const Renderer& renderer);

	[[nodiscard]] bool is_active() const { return active; }

	[[nodiscard]] bool is_browser_open() const { return isBrowserOpen; }

	[[nodiscard]] TileRef get_override(Vector2D worldPosition) const;

	void place(Vector2D worldPosition);
	void erase(Vector2D worldPosition);

	// Direct tile placement bypassing the palette -- used by PrefabLibrary.
	void place_tile(Vector2D worldPosition, TileRef tile);

	[[nodiscard]] bool is_active_map_empty() const;
};
