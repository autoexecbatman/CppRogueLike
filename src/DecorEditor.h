// file: DecorEditor.h
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include "Palette.h"
#include "TileRef.h"
#include "Vector2D.h"

class Renderer;

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
	// Game declares the palette before the editor, so it outlives every use of this reference.
	Palette& palette;

	bool active{ false };
	size_t paletteIndex{ 0 };
	double lastSaveTime{ -10.0 };
	// why the last reported save was refused, until one succeeds
	std::optional<std::string> saveError{};
	// why the open edit was not applied; cleared when an edit opens
	std::optional<std::string> editRefusal{};

	std::string activeKey{};

	std::unordered_map<std::string, std::unordered_map<uint32_t, TileRef>>
		allOverrides{};

	// Sheet browser state
	bool isBrowserOpen{ false };
	int browserSheet{ 0 };
	int browserScroll{ 0 };
	TileRef browserSelected{}; // selected tile, is_valid() == false means none

	// Palette list panel state
	// the selected palette entry, if one is chosen
	std::optional<size_t> listCursor{};
	size_t listScroll{ 0 };

	// Label / symbol editing state
	bool editing{ false };
	std::string labelBuffer{};
	char symbolBuffer{ 0 };
	bool isEditingSymbol{ false }; // true = cursor in symbol field
	bool isLabelAllSelected{ false }; // first keystroke replaces entire label
	// char polled by InputSystem::poll(), fed in before render
	int bufferedCharacter{ 0 };

	// Packs a tile position into one key: row in the high half, column in the
	// low half. Columns wrap at 65536, which no map approaches.
	[[nodiscard]] static uint32_t make_key(Vector2D worldPosition) noexcept
	{
		return (static_cast<uint32_t>(worldPosition.y) << 16) |
			static_cast<uint32_t>(worldPosition.x & 0xFFFF);
	}

	[[nodiscard]] std::unordered_map<uint32_t, TileRef>& current_map();
	[[nodiscard]] const std::unordered_map<uint32_t, TileRef>& current_map()
		const;

	// Normal editor overlay
	void draw_cursor(const Renderer& renderer, Vector2D worldPosition) const;
	void draw_palette_strip(const Renderer& renderer) const;
	void draw_info_bar(const Renderer& renderer, Vector2D worldPosition) const;

	// Sheet browser
	void update_browser(const Renderer& renderer);

	// Takes `tile` out of the palette and keeps the current index inside what is left.
	void remove_from_palette(TileRef tile);

public:
	// The editor edits `editedPalette` in place and saves through it.
	//
	// Example:
	//   DecorEditor editor{ palette };   // loaded or not; place() does nothing while it is empty
	explicit DecorEditor(Palette& editedPalette);

	void toggle();

	void set_char_input(int character) noexcept
	{
		bufferedCharacter = character;
	}

	void cycle_next();
	void cycle_prev();

	void set_active_map(long seed, int dungeonLevel);

	// Saves the palette to the file it was loaded from, for the editor's own save keys. A
	// refusal is kept for save_error() and drawn until a later save succeeds.
	//
	// Example:
	//   editor.save_palette_reporting();   // saved: save_error() -> nullopt, "SAVED!" shows for two seconds
	//   editor.save_palette_reporting();   // file unparseable: save_error() -> "... '...tile_config.json' does not parse: ..."
	void save_palette_reporting();

	// Why the last reported save was refused, or nothing once one succeeds.
	[[nodiscard]] const std::optional<std::string>& save_error() const noexcept
	{
		return saveError;
	}

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
