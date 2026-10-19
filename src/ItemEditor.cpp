// file: ItemEditor.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

#include "BuffType.h"
#include "ContentRegistry.h"
#include "ContentRegistryIO.h"
#include "DiceExpr.h"
#include "GameContext.h"
#include "ItemClassification.h"
#include "ItemEditor.h"
#include "ItemRegistry.h"
#include "MagicalItemEffects.h"
#include "Menu.h"
#include "Paths.h"
#include "Renderer.h"
#include "TargetMode.h"
#include "Weapons.h"

constexpr int LIST_WIDTH = 220;
constexpr int HEADER_HEIGHT = 48;
constexpr int HINT_HEIGHT = 28;
constexpr int FIELD_HEIGHT = 26;
constexpr int ITEM_HEIGHT = 30;
constexpr int LIST_TILE_SIZE = 22;
constexpr int LIST_PAD = 6;
constexpr int PICKER_PAD = 10;
constexpr int PICKER_SUB_HEADER_HEIGHT = 28;
constexpr int PICKER_TILE_TARGET = 64; // drawn cell size the grid aims for

// Largest integer multiple of the sheet's own cell size that fits the target,
// so a 16-pixel tile draws at 4x and a 64-pixel tile at 1x - never blurred.
namespace
{
int picker_tile_size(const Renderer& renderer, int sheetIndex)
{
	const int cellSize = renderer.get_sheet_cell_size(static_cast<TileSheet>(sheetIndex));
	return cellSize * std::max(1, PICKER_TILE_TARGET / cellSize);
}
} // namespace

namespace
{

std::string prettify_key(std::string_view key)
{
	std::string result;
	bool cap_next = true;
	for (char character : key)
	{
		if (character == '_')
		{
			result += ' ';
			cap_next = true;
		}
		else if (cap_next)
		{
			result += static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
			cap_next = false;
		}
		else
		{
			result += character;
		}
	}
	return result;
}

std::string_view consumable_effect_str(ConsumableEffect effect)
{
	switch (effect)
	{
	case ConsumableEffect::NONE:
	{
		return "none";
	}
	case ConsumableEffect::HEAL:
	{
		return "heal";
	}
	case ConsumableEffect::ADD_BUFF:
	{
		return "add_buff";
	}
	case ConsumableEffect::FAIL:
	{
		return "fail";
	}
	}
	return "none";
}

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ItemEditor::enter(GameContext& ctx)
{
	assert(ctx.itemRegistry && "ItemEditor::enter: called without an itemRegistry");

	contentRegistry = ctx.contentRegistry;
	active = true;
	mode = Mode::NORMAL;
	focus = 0;

	keys = ctx.itemRegistry->get_all_keys();

	listCursor = 0;
	listScroll = 0;
	pickerSheet = 0;
	pickerScroll = 0;

	load_working(*ctx.itemRegistry);
}

void ItemEditor::exit(GameContext& ctx)
{
	assert(ctx.itemRegistry && "ItemEditor::exit: called without an itemRegistry");

	commit_working(*ctx.itemRegistry);
	active = false;
	ctx.menus->push_back(make_main_menu(true, ctx));
}

void ItemEditor::tick(GameContext& ctx)
{
	handle_input(ctx);

	render(ctx);
}

// ---------------------------------------------------------------------------
// Working copy
// ---------------------------------------------------------------------------

void ItemEditor::load_working(const ItemRegistry& items)
{
	if (keys.empty())
	{
		return;
	}

	const std::string& key = current_key();
	const ItemParams& params = items.get_params(key);
	working = params;
	workingTile = contentRegistry ? contentRegistry->get_tile(key) : TileRef{};
	fieldCursor = 0;
	fieldScroll = 0;
	mode = Mode::NORMAL;
	editBuffer.clear();
}

void ItemEditor::commit_working(ItemRegistry& items)
{
	if (keys.empty())
	{
		return;
	}
	const std::string& key = current_key();
	items.set_name_category(key, working.name, working.category);
	items.set_params(key, working);
	if (contentRegistry)
	{
		contentRegistry->set_tile(key, workingTile);
	}
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void ItemEditor::handle_input(GameContext& ctx)
{
	assert(ctx.itemRegistry && "ItemEditor::handle_input: called without an itemRegistry");

	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

	if (IsKeyPressed(KEY_ESCAPE))
	{
		exit(ctx);
		return;
	}

	if (ctrl && IsKeyPressed(KEY_S))
	{
		commit_working(*ctx.itemRegistry);
		ctx.itemRegistry->save(Paths::ITEMS);
		if (contentRegistry)
		{
			ContentRegistryIO::save(*contentRegistry, Paths::CONTENT_TILES);
		}
		lastSaveTime = GetTime();
		return;
	}

	if (IsKeyPressed(KEY_TAB))
	{
		focus = 1 - focus;
		return;
	}

	if (mode == Mode::NORMAL)
	{
		handle_normal(ctx);
	}
	else if (mode == Mode::EDIT_STRING)
	{
		handle_edit_string();
	}
	else if (mode == Mode::TILE_PICKER)
	{
		handle_picker(*ctx.renderer);
	}
}

void ItemEditor::handle_normal(const GameContext& ctx)
{
	assert(ctx.itemRegistry && "ItemEditor::handle_normal: called without an itemRegistry");

	ItemRegistry& items = *ctx.itemRegistry;
	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	const Renderer& renderer = *ctx.renderer;
	int screenHeight = renderer.get_screen_height();
	int body_h = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;
	int visible_fields = body_h / FIELD_HEIGHT;
	int total = static_cast<int>(keys.size());

	::Vector2 mouse = GetMousePosition();
	bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

	if (clicked)
	{
		if (mouse.x < LIST_WIDTH)
		{
			focus = 0;
			int idx = listScroll + static_cast<int>(mouse.y - HEADER_HEIGHT) / ITEM_HEIGHT;
			idx = std::clamp(idx, 0, total - 1);
			if (idx != listCursor)
			{
				commit_working(items);
				listCursor = idx;
				load_working(items);
			}
		}
		else if (mode == Mode::NORMAL)
		{
			focus = 1;
			int idx = fieldScroll + static_cast<int>(mouse.y - HEADER_HEIGHT) / FIELD_HEIGHT;
			idx = std::clamp(idx, 0, FIELD_COUNT - 1);
			fieldCursor = idx;
		}
	}

	float wheel = GetMouseWheelMove();
	if (wheel != 0.0f)
	{
		if (mouse.x < LIST_WIDTH)
		{
			listScroll = std::clamp(
				listScroll - static_cast<int>(wheel),
				0,
				std::max(0, total - 1));
		}
		else
		{
			fieldScroll = std::clamp(
				fieldScroll - static_cast<int>(wheel),
				0,
				std::max(0, FIELD_COUNT - visible_fields));
		}
	}

	// Keyboard: list panel
	if (focus == 0)
	{
		if (IsKeyPressed(KEY_UP) && listCursor > 0)
		{
			commit_working(items);
			--listCursor;
			if (listCursor < listScroll)
			{
				listScroll = listCursor;
			}
			load_working(items);
		}
		else if (IsKeyPressed(KEY_DOWN) && listCursor < total - 1)
		{
			commit_working(items);
			++listCursor;
			int vis_list = body_h / 30;
			if (listCursor >= listScroll + vis_list)
			{
				listScroll = listCursor - vis_list + 1;
			}
			load_working(items);
		}
		else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_RIGHT))
		{
			focus = 1;
		}
		else if (IsKeyPressed(KEY_A))
		{
			commit_working(items);
			ItemParams defaults;
			defaults.itemClass = ItemClass::SWORD;
			defaults.pickableType = PickableType::WEAPON;
			defaults.value = 10;
			defaults.baseWeight = 10;
			defaults.levelMin = 1;
			defaults.levelMax = 5;
			defaults.handRequirement = HandRequirement::ONE_HANDED;
			defaults.weaponSize = WeaponSize::MEDIUM;
			const std::string newKey = items.add_custom("New Item", "weapon", defaults);
			keys = items.get_all_keys();
			for (int keyIndex = 0; keyIndex < static_cast<int>(keys.size()); ++keyIndex)
			{
				if (keys[keyIndex] == newKey)
				{
					listCursor = keyIndex;
					break;
				}
			}
			load_working(items);
			focus = 1;
		}
		else if (IsKeyPressed(KEY_DELETE))
		{
			if (!items.is_builtin_key(current_key()))
			{
				// An item deleted before any commit was never given a sprite.
				if (contentRegistry && contentRegistry->all_tiles().contains(current_key()))
				{
					contentRegistry->remove_tile(current_key());
				}
				items.remove_custom(current_key());
				keys = items.get_all_keys();
				listCursor = std::clamp(listCursor, 0, static_cast<int>(keys.size()) - 1);
				load_working(items);
			}
		}
		return;
	}

	// Keyboard: field panel
	int field_max = FIELD_COUNT - 1;

	if (IsKeyPressed(KEY_UP) && fieldCursor > 0)
	{
		--fieldCursor;
		if (fieldCursor < fieldScroll)
		{
			fieldScroll = fieldCursor;
		}
	}
	else if (IsKeyPressed(KEY_DOWN) && fieldCursor < field_max)
	{
		++fieldCursor;
		if (fieldCursor >= fieldScroll + visible_fields)
		{
			fieldScroll = fieldCursor - visible_fields + 1;
		}
	}

	FieldId fid = current_field();

	if (IsKeyPressed(KEY_F2) || (fid == FieldId::TILE && IsKeyPressed(KEY_ENTER)))
	{
		mode = Mode::TILE_PICKER;
		return;
	}

	if (IsKeyPressed(KEY_ENTER))
	{
		if (field_is_string(fid))
		{
			editBuffer = field_value(fid);
			mode = Mode::EDIT_STRING;
		}
		else if (field_is_toggle(fid))
		{
			field_toggle(fid);
		}
		return;
	}

	if (!field_is_string(fid) && !field_is_toggle(fid) && fid != FieldId::TILE)
	{
		int delta = ctrl ? 10 : 1;
		if (IsKeyPressed(KEY_LEFT))
		{
			field_adjust(fid, -delta);
		}
		else if (IsKeyPressed(KEY_RIGHT))
		{
			field_adjust(fid, delta);
		}
	}
}

void ItemEditor::handle_edit_string()
{
	int pressed = GetCharPressed();
	while (pressed != 0)
	{
		if (pressed >= 32 && pressed < 127)
		{
			editBuffer += static_cast<char>(pressed);
		}
		pressed = GetCharPressed();
	}

	if (IsKeyPressed(KEY_BACKSPACE) && !editBuffer.empty())
	{
		editBuffer.pop_back();
	}

	if (IsKeyPressed(KEY_ENTER))
	{
		field_set_string(current_field(), editBuffer);
		mode = Mode::NORMAL;
	}

	if (IsKeyPressed(KEY_ESCAPE))
	{
		mode = Mode::NORMAL;
	}
}

void ItemEditor::handle_picker(const Renderer& renderer)
{
	int total_sheets = renderer.get_loaded_sheet_count();

	auto advance_sheet = [&](int dir)
	{
		for (int sheetsTried = 0; sheetsTried < total_sheets; ++sheetsTried)
		{
			pickerSheet = (pickerSheet + dir + total_sheets) % total_sheets;
			if (renderer.sheet_is_loaded(static_cast<TileSheet>(pickerSheet)))
			{
				break;
			}
		}
		pickerScroll = 0;
	};

	if (!renderer.sheet_is_loaded(static_cast<TileSheet>(pickerSheet)))
	{
		advance_sheet(1);
	}

	if (IsKeyPressed(KEY_LEFT))
	{
		advance_sheet(-1);
	}
	if (IsKeyPressed(KEY_RIGHT))
	{
		advance_sheet(1);
	}
	if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F2))
	{
		mode = Mode::NORMAL;
		return;
	}

	int sheet_rows = renderer.get_sheet_rows(static_cast<TileSheet>(pickerSheet));
	::Vector2 mouse = GetMousePosition();
	bool in_picker = mouse.x >= LIST_WIDTH;

	if (in_picker)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			pickerScroll = std::clamp(
				pickerScroll - static_cast<int>(wheel),
				0,
				std::max(0, sheet_rows - 1));
		}
	}

	int grid_y = HEADER_HEIGHT + PICKER_SUB_HEADER_HEIGHT;

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && in_picker)
	{
		const int pickerTileSize = picker_tile_size(renderer, pickerSheet);
		int col = static_cast<int>(mouse.x - LIST_WIDTH - PICKER_PAD) / (pickerTileSize + 2);
		int row = pickerScroll + static_cast<int>(mouse.y - grid_y) / (pickerTileSize + 2);
		int sheet_cols = renderer.get_sheet_cols(static_cast<TileSheet>(pickerSheet));

		if (col >= 0 && col < sheet_cols && row >= 0 && row < sheet_rows)
		{
			workingTile = TileRef{ static_cast<TileSheet>(pickerSheet), col, row };
			mode = Mode::NORMAL;
		}
	}
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void ItemEditor::render(const GameContext& ctx) const
{
	assert(ctx.itemRegistry && "ItemEditor::render: called without an itemRegistry");

	const Renderer& renderer = *ctx.renderer;
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();

	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 255 });

	render_header(renderer);
	render_list(renderer, *ctx.itemRegistry);

	if (mode == Mode::TILE_PICKER)
	{
		render_picker(renderer);
	}
	else
	{
		render_fields(renderer);
	}

	render_hint(renderer);
}

void ItemEditor::render_header(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	DrawRectangle(0, 0, screenWidth, HEADER_HEIGHT, Color{ 0, 20, 40, 255 });
	renderer.draw_text_color(Vector2D{ 8, 6 }, "ITEM EDITOR", Color{ 180, 255, 180, 255 });
	renderer.draw_text_color(Vector2D{ 8, 26 },
		"Tab:switch focus  Left/Right:adjust  Enter:edit  F2:tile  Ctrl+S:save  Esc:exit",
		Color{ 100, 130, 100, 255 });
}

void ItemEditor::render_list(const Renderer& renderer, const ItemRegistry& items) const
{
	int screenHeight = renderer.get_screen_height();
	int body_y = HEADER_HEIGHT;
	int body_h = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;

	DrawRectangle(0, body_y, LIST_WIDTH, body_h, Color{ 5, 10, 5, 255 });
	DrawLine(LIST_WIDTH, body_y, LIST_WIDTH, body_y + body_h, Color{ 80, 120, 80, 255 });

	int total = static_cast<int>(keys.size());
	int visibleCount = body_h / ITEM_HEIGHT;
	int max_scroll = std::max(0, total - visibleCount);
	int scroll = std::clamp(listScroll, 0, max_scroll);

	::Vector2 mouse = GetMousePosition();

	for (int itemIndex = scroll; itemIndex < total; ++itemIndex)
	{
		int itemY = body_y + (itemIndex - scroll) * ITEM_HEIGHT;
		if (itemY + ITEM_HEIGHT > body_y + body_h)
		{
			break;
		}

		bool is_sel = (itemIndex == listCursor);
		bool hovered = mouse.x >= 0 && mouse.x < LIST_WIDTH && mouse.y >= itemY && mouse.y < itemY + ITEM_HEIGHT;

		Color bgColor{ 0, 0, 0, 0 };
		if (is_sel && focus == 0)
		{
			bgColor = Color{ 0, 60, 0, 200 };
		}
		else if (is_sel)
		{
			bgColor = Color{ 0, 40, 0, 150 };
		}
		else if (hovered)
		{
			bgColor = Color{ 20, 30, 20, 160 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(0, itemY, LIST_WIDTH, ITEM_HEIGHT, bgColor);
		}

		TileRef tile = is_sel
			? workingTile
			: (contentRegistry ? contentRegistry->get_tile(keys[itemIndex]) : TileRef{});
		renderer.draw_tile_screen_sized(Vector2D{ LIST_PAD, itemY + (ITEM_HEIGHT - LIST_TILE_SIZE) / 2 }, tile, LIST_TILE_SIZE);

		const bool isCustom = !items.is_builtin_key(keys[itemIndex]);

		std::string display_name = is_sel
			? working.name
			: std::string{ items.get_params(keys[itemIndex]).name };

		if (display_name.empty())
		{
			display_name = prettify_key(keys[itemIndex]);
		}

		if (isCustom)
		{
			display_name += " *";
		}

		Color textColor = is_sel
			? Color{ 150, 255, 150, 255 }
			: Color{ 200, 200, 200, 255 };

		renderer.draw_text_color(Vector2D{ LIST_PAD + LIST_TILE_SIZE + 4, itemY + (ITEM_HEIGHT - 16) / 2 }, display_name, textColor);
	}
}

void ItemEditor::render_fields(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();
	int panelX = LIST_WIDTH;
	int panelY = HEADER_HEIGHT;
	int fieldsWidth = screenWidth - LIST_WIDTH;
	int fieldsHeight = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;

	DrawRectangle(panelX, panelY, fieldsWidth, fieldsHeight, Color{ 5, 8, 5, 255 });

	int visible = fieldsHeight / FIELD_HEIGHT;
	int max_scroll = std::max(0, FIELD_COUNT - visible);
	int scroll = std::clamp(fieldScroll, 0, max_scroll);

	::Vector2 mouse = GetMousePosition();

	for (int fieldIndex = scroll; fieldIndex < FIELD_COUNT; ++fieldIndex)
	{
		int itemY = panelY + (fieldIndex - scroll) * FIELD_HEIGHT;
		if (itemY + FIELD_HEIGHT > panelY + fieldsHeight)
		{
			break;
		}

		FieldId fid = static_cast<FieldId>(fieldIndex);
		bool is_sel = (fieldIndex == fieldCursor);
		bool hovered = mouse.x >= panelX && mouse.x < screenWidth && mouse.y >= itemY && mouse.y < itemY + FIELD_HEIGHT;

		Color bgColor{ 0, 0, 0, 0 };
		if (is_sel && focus == 1)
		{
			bgColor = Color{ 0, 60, 0, 200 };
		}
		else if (is_sel)
		{
			bgColor = Color{ 0, 40, 0, 140 };
		}
		else if (hovered)
		{
			bgColor = Color{ 10, 20, 10, 120 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(panelX, itemY, fieldsWidth, FIELD_HEIGHT, bgColor);
		}

		Color labelColor = is_sel ? Color{ 150, 255, 150, 255 } : Color{ 150, 150, 150, 255 };
		Color valueColor = is_sel ? Color{ 220, 255, 220, 255 } : Color{ 200, 200, 200, 255 };

		renderer.draw_text_color(Vector2D{ panelX + 12, itemY + (FIELD_HEIGHT - 16) / 2 }, field_label(fid), labelColor);

		if (fid == FieldId::TILE)
		{
			renderer.draw_tile_screen_sized(Vector2D{ screenWidth - LIST_TILE_SIZE - 12, itemY + (FIELD_HEIGHT - LIST_TILE_SIZE) / 2 }, workingTile, LIST_TILE_SIZE);
			if (is_sel)
			{
				DrawRectangleLines(
					screenWidth - LIST_TILE_SIZE - 12,
					itemY + (FIELD_HEIGHT - LIST_TILE_SIZE) / 2,
					LIST_TILE_SIZE,
					LIST_TILE_SIZE,
					Color{ 0, 255, 100, 255 });
			}
		}
		else
		{
			std::string val;
			if (is_sel && mode == Mode::EDIT_STRING)
			{
				val = editBuffer + "_";
			}
			else
			{
				val = field_value(fid);
			}
			int valuePixelX = screenWidth - renderer.measure_text(val) - 16;
			renderer.draw_text_color(Vector2D{ valuePixelX, itemY + (FIELD_HEIGHT - 16) / 2 }, val, valueColor);
		}
	}
}

void ItemEditor::render_picker(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();
	int panelX = LIST_WIDTH;
	int panelWidth = screenWidth - LIST_WIDTH;
	int body_y = HEADER_HEIGHT;
	int body_h = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;

	DrawRectangle(panelX, body_y, panelWidth, body_h, Color{ 5, 8, 24, 255 });

	DrawRectangle(panelX, body_y, panelWidth, PICKER_SUB_HEADER_HEIGHT, Color{ 8, 15, 40, 255 });
	int total_sheets = renderer.get_loaded_sheet_count();
	std::string hdr = std::format(
		"Sheet: {} ({}/{})  --  Left/Right:change  Esc/F2:back",
		renderer.get_sheet_name(static_cast<TileSheet>(pickerSheet)),
		pickerSheet + 1,
		total_sheets);
	renderer.draw_text_color(Vector2D{ panelX + PICKER_PAD, body_y + 6 }, hdr, Color{ 120, 200, 200, 255 });

	int grid_y = body_y + PICKER_SUB_HEADER_HEIGHT;
	int grid_h = body_h - PICKER_SUB_HEADER_HEIGHT;
	int sheet_cols = renderer.get_sheet_cols(static_cast<TileSheet>(pickerSheet));
	int sheet_rows = renderer.get_sheet_rows(static_cast<TileSheet>(pickerSheet));
	const int pickerTileSize = picker_tile_size(renderer, pickerSheet);

	::Vector2 mouse = GetMousePosition();

	BeginScissorMode(panelX, grid_y, panelWidth, grid_h);

	for (int row = pickerScroll; row < sheet_rows; ++row)
	{
		int cellPixelY = grid_y + (row - pickerScroll) * (pickerTileSize + 2);
		if (cellPixelY >= grid_y + grid_h)
		{
			break;
		}

		for (int col = 0; col < sheet_cols; ++col)
		{
			int cellPixelX = panelX + PICKER_PAD + col * (pickerTileSize + 2);
			TileRef tid{ static_cast<TileSheet>(pickerSheet), col, row };

			bool is_cur = (tid == workingTile);
			bool hovered = mouse.x >= cellPixelX && mouse.x < cellPixelX + pickerTileSize && mouse.y >= cellPixelY && mouse.y < cellPixelY + pickerTileSize;

			if (is_cur)
			{
				DrawRectangle(cellPixelX, cellPixelY, pickerTileSize, pickerTileSize, Color{ 0, 60, 0, 220 });
			}
			else if (hovered)
			{
				DrawRectangle(cellPixelX, cellPixelY, pickerTileSize, pickerTileSize, Color{ 20, 40, 20, 160 });
			}

			renderer.draw_tile_screen_sized(Vector2D{ cellPixelX, cellPixelY }, tid, pickerTileSize);

			if (is_cur)
			{
				DrawRectangleLines(cellPixelX, cellPixelY, pickerTileSize, pickerTileSize, Color{ 0, 255, 100, 255 });
			}
		}
	}

	EndScissorMode();
}

void ItemEditor::render_hint(const Renderer& renderer) const
{
	int screenWidth = renderer.get_screen_width();
	int screenHeight = renderer.get_screen_height();
	int hint_y = screenHeight - HINT_HEIGHT;

	DrawRectangle(0, hint_y, screenWidth, HINT_HEIGHT, Color{ 0, 20, 40, 255 });

	std::string msg;
	bool saved_flash = (GetTime() - lastSaveTime) < 2.0;

	if (saved_flash)
	{
		msg = "Saved!";
	}
	else if (mode == Mode::EDIT_STRING)
	{
		msg = "Typing  --  Enter:confirm  Esc:cancel";
	}
	else if (mode == Mode::TILE_PICKER)
	{
		msg = "Tile Picker  --  Left/Right:sheet  Click:assign  Esc/F2:back";
	}
	else if (focus == 0)
	{
		msg = "[LIST] Up/Down:navigate  A:add new  Del:remove custom  Enter:edit fields  Tab:switch  Ctrl+S:save  Esc:exit";
	}
	else
	{
		msg = "[FIELDS] Up/Down:navigate  Left/Right:adjust  Enter:edit/toggle  F2:tile  Tab:switch  Ctrl+S:save  Esc:exit";
	}

	Color hintColor = saved_flash ? Color{ 100, 255, 100, 255 } : Color{ 120, 160, 120, 255 };
	renderer.draw_text_color(Vector2D{ 8, hint_y + 6 }, msg, hintColor);
}

// ---------------------------------------------------------------------------
// Field accessors
// ---------------------------------------------------------------------------

const std::string& ItemEditor::current_key() const
{
	if (keys.empty())
	{
		throw std::out_of_range("ItemEditor::current_key -- key list is empty");
	}
	return keys[listCursor];
}

ItemEditor::FieldId ItemEditor::current_field() const
{
	return static_cast<FieldId>(fieldCursor);
}

std::string ItemEditor::field_label(FieldId field) const
{
	switch (field)
	{
	case FieldId::NAME:
	{
		return "Name";
	}
	case FieldId::CATEGORY:
	{
		return "Category";
	}
	case FieldId::ITEM_CLASS:
	{
		return "Item Class";
	}
	case FieldId::PICKABLE_TYPE:
	{
		return "Pickable Type";
	}
	case FieldId::COLOR:
	{
		return "Color";
	}
	case FieldId::VALUE:
	{
		return "Value";
	}
	case FieldId::BASE_WEIGHT:
	{
		return "Spawn Weight";
	}
	case FieldId::WEIGHT:
	{
		return "Carry Weight";
	}
	case FieldId::LEVEL_MIN:
	{
		return "Level Min";
	}
	case FieldId::LEVEL_MAX:
	{
		return "Level Max";
	}
	case FieldId::LEVEL_SCALING:
	{
		return "Level Scaling";
	}
	case FieldId::CONSUMABLE_EFFECT:
	{
		return "Consumable Effect";
	}
	case FieldId::CONSUMABLE_BUFF:
	{
		return "Consumable Buff";
	}
	case FieldId::CONSUMABLE_AMT_DICE:
	{
		return "Amount Dice";
	}
	case FieldId::CONSUMABLE_AMT_SIDES:
	{
		return "Amount Sides";
	}
	case FieldId::CONSUMABLE_AMT_BONUS:
	{
		return "Amount Bonus";
	}
	case FieldId::DURATION_TURNS_DICE:
	{
		return "Potion Turns Dice";
	}
	case FieldId::DURATION_TURNS_SIDES:
	{
		return "Potion Turns Sides";
	}
	case FieldId::DURATION_TURNS_BONUS:
	{
		return "Potion Turns Bonus";
	}
	case FieldId::DURATION:
	{
		return "Scroll Rounds";
	}
	case FieldId::TARGET_MODE:
	{
		return "Target Mode";
	}
	case FieldId::SCROLL_ANIM:
	{
		return "Scroll Animation";
	}
	case FieldId::RANGE:
	{
		return "Range";
	}
	case FieldId::DAMAGE:
	{
		return "Damage";
	}
	case FieldId::CONFUSE_TURNS:
	{
		return "Confuse Turns";
	}
	case FieldId::RANGED:
	{
		return "Ranged";
	}
	case FieldId::HAND_REQUIREMENT:
	{
		return "Hand Requirement";
	}
	case FieldId::WEAPON_SIZE:
	{
		return "Weapon Size";
	}
	case FieldId::STRENGTH_RATING:
	{
		return "Made For STR";
	}
	case FieldId::AC_BONUS:
	{
		return "AC Bonus";
	}
	case FieldId::EFFECT:
	{
		return "Effect";
	}
	case FieldId::EFFECT_BONUS:
	{
		return "Effect Bonus";
	}
	case FieldId::STR_BONUS:
	{
		return "STR Bonus";
	}
	case FieldId::DEX_BONUS:
	{
		return "DEX Bonus";
	}
	case FieldId::CON_BONUS:
	{
		return "CON Bonus";
	}
	case FieldId::INT_BONUS:
	{
		return "INT Bonus";
	}
	case FieldId::WIS_BONUS:
	{
		return "WIS Bonus";
	}
	case FieldId::CHA_BONUS:
	{
		return "CHA Bonus";
	}
	case FieldId::APPLICATION:
	{
		return "Application";
	}
	case FieldId::EXCEPTIONAL_STRENGTH:
	{
		return "Exceptional STR";
	}
	case FieldId::NUTRITION:
	{
		return "Nutrition Value";
	}
	case FieldId::TILE:
	{
		return "Tile";
	}
	default:
	{
		return "???";
	}
	}
}

std::string ItemEditor::field_value(FieldId field) const
{
	const ItemParams& params = working;
	switch (field)
	{
	case FieldId::NAME:
	{
		return working.name;
	}
	case FieldId::CATEGORY:
	{
		return working.category;
	}
	case FieldId::ITEM_CLASS:
	{
		return std::string{ encode_item_class(params.itemClass) };
	}
	case FieldId::PICKABLE_TYPE:
	{
		return std::string{ encode_pickable_type(params.pickableType) };
	}
	case FieldId::COLOR:
	{
		return std::string{ color_pair_name(params.color) };
	}
	case FieldId::VALUE:
	{
		return std::format("{}", params.value);
	}
	case FieldId::BASE_WEIGHT:
	{
		return std::format("{}", params.baseWeight);
	}
	case FieldId::WEIGHT:
	{
		return std::format("{}", params.weight);
	}
	case FieldId::LEVEL_MIN:
	{
		return std::format("{}", params.levelMin);
	}
	case FieldId::LEVEL_MAX:
	{
		return std::format("{}", params.levelMax);
	}
	case FieldId::LEVEL_SCALING:
	{
		return std::format("{:.2f}", params.levelScaling);
	}
	case FieldId::CONSUMABLE_EFFECT:
	{
		return std::string{ consumable_effect_str(params.consumableEffect) };
	}
	case FieldId::CONSUMABLE_BUFF:
	{
		return std::string{ encode_buff_type(params.consumableBuffType) };
	}
	case FieldId::CONSUMABLE_AMT_DICE:
	{
		return std::format("{} ({})", params.consumableAmount.num, to_text(params.consumableAmount));
	}
	case FieldId::CONSUMABLE_AMT_SIDES:
	{
		return std::format("{}", params.consumableAmount.sides);
	}
	case FieldId::CONSUMABLE_AMT_BONUS:
	{
		return std::format("{}", params.consumableAmount.bonus);
	}
	case FieldId::DURATION_TURNS_DICE:
	{
		return std::format("{} ({})", params.durationTurns.num, to_text(params.durationTurns));
	}
	case FieldId::DURATION_TURNS_SIDES:
	{
		return std::format("{}", params.durationTurns.sides);
	}
	case FieldId::DURATION_TURNS_BONUS:
	{
		return std::format("{}", params.durationTurns.bonus);
	}
	case FieldId::DURATION:
	{
		return std::format("{}", params.duration);
	}
	case FieldId::TARGET_MODE:
	{
		return std::string{ encode_target_mode(params.targetMode) };
	}
	case FieldId::SCROLL_ANIM:
	{
		return std::string{ encode_scroll_animation(params.scrollAnimation) };
	}
	case FieldId::RANGE:
	{
		return std::format("{}", params.range);
	}
	case FieldId::DAMAGE:
	{
		return std::format("{}", params.damage);
	}
	case FieldId::CONFUSE_TURNS:
	{
		return std::format("{}", params.confuseTurns);
	}
	case FieldId::RANGED:
	{
		return params.ranged ? "yes" : "no";
	}
	case FieldId::HAND_REQUIREMENT:
	{
		return std::string{ encode_hand_requirement(params.handRequirement) };
	}
	case FieldId::WEAPON_SIZE:
	{
		return std::string{ encode_weapon_size(params.weaponSize) };
	}
	case FieldId::STRENGTH_RATING:
	{
		return std::format("{}", params.strengthRating);
	}
	case FieldId::AC_BONUS:
	{
		return std::format("{}", params.acBonus);
	}
	case FieldId::EFFECT:
	{
		return std::string{ encode_magical_effect(params.effect) };
	}
	case FieldId::EFFECT_BONUS:
	{
		return std::format("{}", params.effectBonus);
	}
	case FieldId::STR_BONUS:
	{
		return std::format("{}", params.strBonus);
	}
	case FieldId::DEX_BONUS:
	{
		return std::format("{}", params.dexBonus);
	}
	case FieldId::CON_BONUS:
	{
		return std::format("{}", params.conBonus);
	}
	case FieldId::INT_BONUS:
	{
		return std::format("{}", params.intBonus);
	}
	case FieldId::WIS_BONUS:
	{
		return std::format("{}", params.wisBonus);
	}
	case FieldId::CHA_BONUS:
	{
		return std::format("{}", params.chaBonus);
	}
	case FieldId::APPLICATION:
	{
		return std::string{ encode_buff_application(params.application) };
	}
	case FieldId::EXCEPTIONAL_STRENGTH:
	{
		return std::format("{}", params.exceptionalStrength);
	}
	case FieldId::NUTRITION:
	{
		return std::format("{}", params.nutritionValue);
	}
	case FieldId::TILE:
	{
		return "(tile)";
	}
	default:
	{
		return "";
	}
	}
}

bool ItemEditor::field_is_string(FieldId field) const
{
	return field == FieldId::NAME || field == FieldId::CATEGORY;
}

bool ItemEditor::field_is_toggle(FieldId field) const
{
	return field == FieldId::ITEM_CLASS || field == FieldId::PICKABLE_TYPE || field == FieldId::CONSUMABLE_EFFECT || field == FieldId::CONSUMABLE_BUFF || field == FieldId::TARGET_MODE || field == FieldId::SCROLL_ANIM || field == FieldId::RANGED || field == FieldId::HAND_REQUIREMENT || field == FieldId::WEAPON_SIZE || field == FieldId::EFFECT || field == FieldId::COLOR || field == FieldId::APPLICATION;
}

void ItemEditor::field_adjust(FieldId field, int delta)
{
	auto clamp_val = [](int value, int step, int low, int high) -> int
	{
		return std::clamp(value + step, low, high);
	};

	ItemParams& params = working;
	switch (field)
	{
	case FieldId::VALUE:
	{
		params.value = clamp_val(params.value, delta, 0, 99999);
		break;
	}
	case FieldId::BASE_WEIGHT:
	{
		params.baseWeight = clamp_val(params.baseWeight, delta, 0, 100);
		break;
	}
	case FieldId::WEIGHT:
	{
		params.weight = clamp_val(params.weight, delta, 0, 1000);
		break;
	}
	case FieldId::LEVEL_MIN:
	{
		params.levelMin = clamp_val(params.levelMin, delta, 1, 20);
		break;
	}
	case FieldId::LEVEL_MAX:
	{
		params.levelMax = clamp_val(params.levelMax, delta, 0, 20);
		break;
	}
	case FieldId::LEVEL_SCALING:
	{
		float raw = params.levelScaling + static_cast<float>(delta) * 0.01f;
		params.levelScaling = std::clamp(raw, 0.0f, 5.0f);
		break;
	}
	case FieldId::CONSUMABLE_AMT_DICE:
	{
		// No dice is a fixed amount; any dice need a die to roll, and a d2 is the least.
		params.consumableAmount.num = clamp_val(params.consumableAmount.num, delta, 0, 99);
		params.consumableAmount.sides = params.consumableAmount.num == 0 ? 0 : std::max(params.consumableAmount.sides, 2);
		break;
	}
	case FieldId::CONSUMABLE_AMT_SIDES:
	{
		params.consumableAmount.sides = params.consumableAmount.num == 0 ? 0 : clamp_val(params.consumableAmount.sides, delta, 2, 20);
		break;
	}
	case FieldId::CONSUMABLE_AMT_BONUS:
	{
		params.consumableAmount.bonus = clamp_val(params.consumableAmount.bonus, delta, 0, 9999);
		break;
	}
	case FieldId::DURATION_TURNS_DICE:
	{
		// No dice is a fixed duration; any dice need a die to roll, and a d2 is the least.
		params.durationTurns.num = clamp_val(params.durationTurns.num, delta, 0, 99);
		params.durationTurns.sides = params.durationTurns.num == 0 ? 0 : std::max(params.durationTurns.sides, 2);
		break;
	}
	case FieldId::DURATION_TURNS_SIDES:
	{
		params.durationTurns.sides = params.durationTurns.num == 0 ? 0 : clamp_val(params.durationTurns.sides, delta, 2, 20);
		break;
	}
	case FieldId::DURATION_TURNS_BONUS:
	{
		params.durationTurns.bonus = clamp_val(params.durationTurns.bonus, delta, 0, 99);
		break;
	}
	case FieldId::DURATION:
	{
		params.duration = clamp_val(params.duration, delta, 0, 9999);
		break;
	}
	case FieldId::RANGE:
	{
		params.range = clamp_val(params.range, delta, 0, 50);
		break;
	}
	case FieldId::DAMAGE:
	{
		params.damage = clamp_val(params.damage, delta, 0, 999);
		break;
	}
	case FieldId::CONFUSE_TURNS:
	{
		params.confuseTurns = clamp_val(params.confuseTurns, delta, 0, 999);
		break;
	}
	case FieldId::STRENGTH_RATING:
	{
		// 0 is an ordinary weapon; a rating is a Strength score, and Table 1 stops at 25.
		params.strengthRating = clamp_val(params.strengthRating, delta, 0, 25);
		break;
	}
	case FieldId::AC_BONUS:
	{
		params.acBonus = clamp_val(params.acBonus, delta, -10, 10);
		break;
	}
	case FieldId::EFFECT_BONUS:
	{
		params.effectBonus = clamp_val(params.effectBonus, delta, 0, 20);
		break;
	}
	case FieldId::STR_BONUS:
	{
		params.strBonus = clamp_val(params.strBonus, delta, -18, 18);
		break;
	}
	case FieldId::DEX_BONUS:
	{
		params.dexBonus = clamp_val(params.dexBonus, delta, -18, 18);
		break;
	}
	case FieldId::CON_BONUS:
	{
		params.conBonus = clamp_val(params.conBonus, delta, -18, 18);
		break;
	}
	case FieldId::INT_BONUS:
	{
		params.intBonus = clamp_val(params.intBonus, delta, -18, 18);
		break;
	}
	case FieldId::WIS_BONUS:
	{
		params.wisBonus = clamp_val(params.wisBonus, delta, -18, 18);
		break;
	}
	case FieldId::CHA_BONUS:
	{
		params.chaBonus = clamp_val(params.chaBonus, delta, -18, 18);
		break;
	}
	case FieldId::EXCEPTIONAL_STRENGTH:
	{
		params.exceptionalStrength = clamp_val(params.exceptionalStrength, delta, 0, 100);
		break;
	}
	case FieldId::NUTRITION:
	{
		params.nutritionValue = clamp_val(params.nutritionValue, delta, 0, 9999);
		break;
	}
	default:
	{
		break;
	}
	}
}

void ItemEditor::field_toggle(FieldId field)
{
	ItemParams& params = working;
	switch (field)
	{
	case FieldId::COLOR:
	{
		params.color = next_color_pair(params.color);
		break;
	}
	case FieldId::ITEM_CLASS:
	{
		params.itemClass = next_item_class(params.itemClass);
		break;
	}
	case FieldId::PICKABLE_TYPE:
	{
		params.pickableType = next_pickable_type(params.pickableType);
		break;
	}
	case FieldId::CONSUMABLE_EFFECT:
	{
		params.consumableEffect = next_consumable_effect(params.consumableEffect);
		break;
	}
	case FieldId::CONSUMABLE_BUFF:
	{
		params.consumableBuffType = next_buff_type(params.consumableBuffType);
		break;
	}
	case FieldId::TARGET_MODE:
	{
		params.targetMode = next_target_mode(params.targetMode);
		break;
	}
	case FieldId::SCROLL_ANIM:
	{
		params.scrollAnimation = next_scroll_animation(params.scrollAnimation);
		break;
	}
	case FieldId::RANGED:
	{
		params.ranged = !params.ranged;
		break;
	}
	case FieldId::HAND_REQUIREMENT:
	{
		params.handRequirement = next_hand_requirement(params.handRequirement);
		break;
	}
	case FieldId::WEAPON_SIZE:
	{
		params.weaponSize = next_weapon_size(params.weaponSize);
		break;
	}
	case FieldId::EFFECT:
	{
		params.effect = next_magical_effect(params.effect);
		break;
	}
	case FieldId::APPLICATION:
	{
		params.application = params.application == BuffApplication::ADD ? BuffApplication::SET : BuffApplication::ADD;
		break;
	}
	default:
	{
		break;
	}
	}
}

void ItemEditor::field_set_string(FieldId field, std::string val)
{
	switch (field)
	{
	case FieldId::NAME:
	{
		working.name = std::move(val);
		break;
	}
	case FieldId::CATEGORY:
	{
		working.category = std::move(val);
		break;
	}
	default:
	{
		break;
	}
	}
}

// end of file: ItemEditor.cpp
