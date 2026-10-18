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
	for (char c : key)
	{
		if (c == '_')
		{
			result += ' ';
			cap_next = true;
		}
		else if (cap_next)
		{
			result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
			cap_next = false;
		}
		else
		{
			result += c;
		}
	}
	return result;
}

std::string_view consumable_effect_str(ConsumableEffect e)
{
	switch (e)
	{
	case ConsumableEffect::NONE:
		return "none";
	case ConsumableEffect::HEAL:
		return "heal";
	case ConsumableEffect::ADD_BUFF:
		return "add_buff";
	case ConsumableEffect::FAIL:
		return "fail";
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
	const ItemParams& p = items.get_params(key);
	working = p;
	workingName = std::string{ p.name };
	workingCategory = std::string{ p.category };
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
	items.set_name_category(key, workingName, workingCategory);
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
	const Renderer& r = *ctx.renderer;
	int screenHeight = r.get_screen_height();
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
			for (int i = 0; i < static_cast<int>(keys.size()); ++i)
			{
				if (keys[i] == newKey)
				{
					listCursor = i;
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
	int ch = GetCharPressed();
	while (ch != 0)
	{
		if (ch >= 32 && ch < 127)
		{
			editBuffer += static_cast<char>(ch);
		}
		ch = GetCharPressed();
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

void ItemEditor::handle_picker(const Renderer& r)
{
	int total_sheets = r.get_loaded_sheet_count();

	auto advance_sheet = [&](int dir)
	{
		for (int i = 0; i < total_sheets; ++i)
		{
			pickerSheet = (pickerSheet + dir + total_sheets) % total_sheets;
			if (r.sheet_is_loaded(static_cast<TileSheet>(pickerSheet)))
			{
				break;
			}
		}
		pickerScroll = 0;
	};

	if (!r.sheet_is_loaded(static_cast<TileSheet>(pickerSheet)))
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

	int sheet_rows = r.get_sheet_rows(static_cast<TileSheet>(pickerSheet));
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
		const int pickerTileSize = picker_tile_size(r, pickerSheet);
		int col = static_cast<int>(mouse.x - LIST_WIDTH - PICKER_PAD) / (pickerTileSize + 2);
		int row = pickerScroll + static_cast<int>(mouse.y - grid_y) / (pickerTileSize + 2);
		int sheet_cols = r.get_sheet_cols(static_cast<TileSheet>(pickerSheet));

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

	const Renderer& r = *ctx.renderer;
	int screenWidth = r.get_screen_width();
	int screenHeight = r.get_screen_height();

	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 255 });

	render_header(r);
	render_list(r, *ctx.itemRegistry);

	if (mode == Mode::TILE_PICKER)
	{
		render_picker(r);
	}
	else
	{
		render_fields(r);
	}

	render_hint(r);
}

void ItemEditor::render_header(const Renderer& r) const
{
	int screenWidth = r.get_screen_width();
	DrawRectangle(0, 0, screenWidth, HEADER_HEIGHT, Color{ 0, 20, 40, 255 });
	r.draw_text_color(Vector2D{ 8, 6 }, "ITEM EDITOR", Color{ 180, 255, 180, 255 });
	r.draw_text_color(Vector2D{ 8, 26 },
		"Tab:switch focus  Left/Right:adjust  Enter:edit  F2:tile  Ctrl+S:save  Esc:exit",
		Color{ 100, 130, 100, 255 });
}

void ItemEditor::render_list(const Renderer& r, const ItemRegistry& items) const
{
	int screenHeight = r.get_screen_height();
	int body_y = HEADER_HEIGHT;
	int body_h = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;

	DrawRectangle(0, body_y, LIST_WIDTH, body_h, Color{ 5, 10, 5, 255 });
	DrawLine(LIST_WIDTH, body_y, LIST_WIDTH, body_y + body_h, Color{ 80, 120, 80, 255 });

	int total = static_cast<int>(keys.size());
	int visibleCount = body_h / ITEM_HEIGHT;
	int max_scroll = std::max(0, total - visibleCount);
	int scroll = std::clamp(listScroll, 0, max_scroll);

	::Vector2 mouse = GetMousePosition();

	for (int i = scroll; i < total; ++i)
	{
		int itemY = body_y + (i - scroll) * ITEM_HEIGHT;
		if (itemY + ITEM_HEIGHT > body_y + body_h)
		{
			break;
		}

		bool is_sel = (i == listCursor);
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
			: (contentRegistry ? contentRegistry->get_tile(keys[i]) : TileRef{});
		r.draw_tile_screen_sized(Vector2D{ LIST_PAD, itemY + (ITEM_HEIGHT - LIST_TILE_SIZE) / 2 }, tile, LIST_TILE_SIZE);

		const bool isCustom = !items.is_builtin_key(keys[i]);

		std::string display_name = is_sel
			? workingName
			: std::string{ items.get_params(keys[i]).name };

		if (display_name.empty())
		{
			display_name = prettify_key(keys[i]);
		}

		if (isCustom)
		{
			display_name += " *";
		}

		Color textColor = is_sel
			? Color{ 150, 255, 150, 255 }
			: Color{ 200, 200, 200, 255 };

		r.draw_text_color(Vector2D{ LIST_PAD + LIST_TILE_SIZE + 4, itemY + (ITEM_HEIGHT - 16) / 2 }, display_name, textColor);
	}
}

void ItemEditor::render_fields(const Renderer& r) const
{
	int screenWidth = r.get_screen_width();
	int screenHeight = r.get_screen_height();
	int panelX = LIST_WIDTH;
	int panelY = HEADER_HEIGHT;
	int fieldsWidth = screenWidth - LIST_WIDTH;
	int fieldsHeight = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;

	DrawRectangle(panelX, panelY, fieldsWidth, fieldsHeight, Color{ 5, 8, 5, 255 });

	int visible = fieldsHeight / FIELD_HEIGHT;
	int max_scroll = std::max(0, FIELD_COUNT - visible);
	int scroll = std::clamp(fieldScroll, 0, max_scroll);

	::Vector2 mouse = GetMousePosition();

	for (int i = scroll; i < FIELD_COUNT; ++i)
	{
		int itemY = panelY + (i - scroll) * FIELD_HEIGHT;
		if (itemY + FIELD_HEIGHT > panelY + fieldsHeight)
		{
			break;
		}

		FieldId fid = static_cast<FieldId>(i);
		bool is_sel = (i == fieldCursor);
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

		r.draw_text_color(Vector2D{ panelX + 12, itemY + (FIELD_HEIGHT - 16) / 2 }, field_label(fid), labelColor);

		if (fid == FieldId::TILE)
		{
			r.draw_tile_screen_sized(Vector2D{ screenWidth - LIST_TILE_SIZE - 12, itemY + (FIELD_HEIGHT - LIST_TILE_SIZE) / 2 }, workingTile, LIST_TILE_SIZE);
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
			int vx = screenWidth - r.measure_text(val) - 16;
			r.draw_text_color(Vector2D{ vx, itemY + (FIELD_HEIGHT - 16) / 2 }, val, valueColor);
		}
	}
}

void ItemEditor::render_picker(const Renderer& r) const
{
	int screenWidth = r.get_screen_width();
	int screenHeight = r.get_screen_height();
	int panelX = LIST_WIDTH;
	int panelWidth = screenWidth - LIST_WIDTH;
	int body_y = HEADER_HEIGHT;
	int body_h = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;

	DrawRectangle(panelX, body_y, panelWidth, body_h, Color{ 5, 8, 24, 255 });

	DrawRectangle(panelX, body_y, panelWidth, PICKER_SUB_HEADER_HEIGHT, Color{ 8, 15, 40, 255 });
	int total_sheets = r.get_loaded_sheet_count();
	std::string hdr = std::format(
		"Sheet: {} ({}/{})  --  Left/Right:change  Esc/F2:back",
		r.get_sheet_name(static_cast<TileSheet>(pickerSheet)),
		pickerSheet + 1,
		total_sheets);
	r.draw_text_color(Vector2D{ panelX + PICKER_PAD, body_y + 6 }, hdr, Color{ 120, 200, 200, 255 });

	int grid_y = body_y + PICKER_SUB_HEADER_HEIGHT;
	int grid_h = body_h - PICKER_SUB_HEADER_HEIGHT;
	int sheet_cols = r.get_sheet_cols(static_cast<TileSheet>(pickerSheet));
	int sheet_rows = r.get_sheet_rows(static_cast<TileSheet>(pickerSheet));
	const int pickerTileSize = picker_tile_size(r, pickerSheet);

	::Vector2 mouse = GetMousePosition();

	BeginScissorMode(panelX, grid_y, panelWidth, grid_h);

	for (int row = pickerScroll; row < sheet_rows; ++row)
	{
		int py = grid_y + (row - pickerScroll) * (pickerTileSize + 2);
		if (py >= grid_y + grid_h)
		{
			break;
		}

		for (int col = 0; col < sheet_cols; ++col)
		{
			int px = panelX + PICKER_PAD + col * (pickerTileSize + 2);
			TileRef tid{ static_cast<TileSheet>(pickerSheet), col, row };

			bool is_cur = (tid == workingTile);
			bool hovered = mouse.x >= px && mouse.x < px + pickerTileSize && mouse.y >= py && mouse.y < py + pickerTileSize;

			if (is_cur)
			{
				DrawRectangle(px, py, pickerTileSize, pickerTileSize, Color{ 0, 60, 0, 220 });
			}
			else if (hovered)
			{
				DrawRectangle(px, py, pickerTileSize, pickerTileSize, Color{ 20, 40, 20, 160 });
			}

			r.draw_tile_screen_sized(Vector2D{ px, py }, tid, pickerTileSize);

			if (is_cur)
			{
				DrawRectangleLines(px, py, pickerTileSize, pickerTileSize, Color{ 0, 255, 100, 255 });
			}
		}
	}

	EndScissorMode();
}

void ItemEditor::render_hint(const Renderer& r) const
{
	int screenWidth = r.get_screen_width();
	int screenHeight = r.get_screen_height();
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
	r.draw_text_color(Vector2D{ 8, hint_y + 6 }, msg, hintColor);
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

std::string ItemEditor::field_label(FieldId f) const
{
	switch (f)
	{
	case FieldId::NAME:
		return "Name";
	case FieldId::CATEGORY:
		return "Category";
	case FieldId::ITEM_CLASS:
		return "Item Class";
	case FieldId::PICKABLE_TYPE:
		return "Pickable Type";
	case FieldId::COLOR:
		return "Color";
	case FieldId::VALUE:
		return "Value";
	case FieldId::BASE_WEIGHT:
		return "Spawn Weight";
	case FieldId::WEIGHT:
		return "Carry Weight";
	case FieldId::LEVEL_MIN:
		return "Level Min";
	case FieldId::LEVEL_MAX:
		return "Level Max";
	case FieldId::LEVEL_SCALING:
		return "Level Scaling";
	case FieldId::CONSUMABLE_EFFECT:
		return "Consumable Effect";
	case FieldId::CONSUMABLE_BUFF:
		return "Consumable Buff";
	case FieldId::CONSUMABLE_AMT:
		return "Consumable Amount";
	case FieldId::DURATION:
		return "Duration";
	case FieldId::TARGET_MODE:
		return "Target Mode";
	case FieldId::SCROLL_ANIM:
		return "Scroll Animation";
	case FieldId::RANGE:
		return "Range";
	case FieldId::DAMAGE:
		return "Damage";
	case FieldId::CONFUSE_TURNS:
		return "Confuse Turns";
	case FieldId::RANGED:
		return "Ranged";
	case FieldId::HAND_REQUIREMENT:
		return "Hand Requirement";
	case FieldId::WEAPON_SIZE:
		return "Weapon Size";
	case FieldId::STRENGTH_RATING:
	{
		return "Made For STR";
	}
	case FieldId::AC_BONUS:
		return "AC Bonus";
	case FieldId::EFFECT:
		return "Effect";
	case FieldId::EFFECT_BONUS:
		return "Effect Bonus";
	case FieldId::STR_BONUS:
		return "STR Bonus";
	case FieldId::DEX_BONUS:
		return "DEX Bonus";
	case FieldId::CON_BONUS:
		return "CON Bonus";
	case FieldId::INT_BONUS:
		return "INT Bonus";
	case FieldId::WIS_BONUS:
		return "WIS Bonus";
	case FieldId::CHA_BONUS:
		return "CHA Bonus";
	case FieldId::IS_SET_MODE:
		return "Is Set Mode";
	case FieldId::EXCEPTIONAL_STRENGTH:
	{
		return "Exceptional STR";
	}
	case FieldId::NUTRITION:
		return "Nutrition Value";
	case FieldId::TILE:
		return "Tile";
	default:
		return "???";
	}
}

std::string ItemEditor::field_value(FieldId f) const
{
	const ItemParams& p = working;
	switch (f)
	{
	case FieldId::NAME:
		return workingName;
	case FieldId::CATEGORY:
		return workingCategory;
	case FieldId::ITEM_CLASS:
		return std::string{ encode_item_class(p.itemClass) };
	case FieldId::PICKABLE_TYPE:
		return std::string{ encode_pickable_type(p.pickableType) };
	case FieldId::COLOR:
		return std::string{ color_pair_name(p.color) };
	case FieldId::VALUE:
		return std::format("{}", p.value);
	case FieldId::BASE_WEIGHT:
		return std::format("{}", p.baseWeight);
	case FieldId::WEIGHT:
		return std::format("{}", p.weight);
	case FieldId::LEVEL_MIN:
		return std::format("{}", p.levelMin);
	case FieldId::LEVEL_MAX:
		return std::format("{}", p.levelMax);
	case FieldId::LEVEL_SCALING:
		return std::format("{:.2f}", p.levelScaling);
	case FieldId::CONSUMABLE_EFFECT:
		return std::string{ consumable_effect_str(p.consumableEffect) };
	case FieldId::CONSUMABLE_BUFF:
		return std::string{ encode_buff_type(p.consumableBuffType) };
	case FieldId::CONSUMABLE_AMT:
		return std::format("{}", p.consumableAmount);
	case FieldId::DURATION:
		return std::format("{}", p.duration);
	case FieldId::TARGET_MODE:
		return std::string{ encode_target_mode(p.targetMode) };
	case FieldId::SCROLL_ANIM:
		return std::string{ encode_scroll_animation(p.scrollAnimation) };
	case FieldId::RANGE:
		return std::format("{}", p.range);
	case FieldId::DAMAGE:
		return std::format("{}", p.damage);
	case FieldId::CONFUSE_TURNS:
		return std::format("{}", p.confuseTurns);
	case FieldId::RANGED:
		return p.ranged ? "yes" : "no";
	case FieldId::HAND_REQUIREMENT:
		return std::string{ encode_hand_requirement(p.handRequirement) };
	case FieldId::WEAPON_SIZE:
		return std::string{ encode_weapon_size(p.weaponSize) };
	case FieldId::STRENGTH_RATING:
	{
		return std::format("{}", p.strengthRating);
	}
	case FieldId::AC_BONUS:
		return std::format("{}", p.acBonus);
	case FieldId::EFFECT:
		return std::string{ encode_magical_effect(p.effect) };
	case FieldId::EFFECT_BONUS:
		return std::format("{}", p.effectBonus);
	case FieldId::STR_BONUS:
		return std::format("{}", p.strBonus);
	case FieldId::DEX_BONUS:
		return std::format("{}", p.dexBonus);
	case FieldId::CON_BONUS:
		return std::format("{}", p.conBonus);
	case FieldId::INT_BONUS:
		return std::format("{}", p.intBonus);
	case FieldId::WIS_BONUS:
		return std::format("{}", p.wisBonus);
	case FieldId::CHA_BONUS:
		return std::format("{}", p.chaBonus);
	case FieldId::IS_SET_MODE:
		return p.isSetMode ? "yes" : "no";
	case FieldId::EXCEPTIONAL_STRENGTH:
	{
		return std::format("{}", p.exceptionalStrength);
	}
	case FieldId::NUTRITION:
		return std::format("{}", p.nutritionValue);
	case FieldId::TILE:
		return "(tile)";
	default:
		return "";
	}
}

bool ItemEditor::field_is_string(FieldId f) const
{
	return f == FieldId::NAME || f == FieldId::CATEGORY;
}

bool ItemEditor::field_is_toggle(FieldId f) const
{
	return f == FieldId::ITEM_CLASS || f == FieldId::PICKABLE_TYPE || f == FieldId::CONSUMABLE_EFFECT || f == FieldId::CONSUMABLE_BUFF || f == FieldId::TARGET_MODE || f == FieldId::SCROLL_ANIM || f == FieldId::RANGED || f == FieldId::HAND_REQUIREMENT || f == FieldId::WEAPON_SIZE || f == FieldId::EFFECT || f == FieldId::COLOR || f == FieldId::IS_SET_MODE;
}

void ItemEditor::field_adjust(FieldId f, int delta)
{
	auto clamp_val = [](int v, int d, int lo, int hi) -> int
	{
		return std::clamp(v + d, lo, hi);
	};

	ItemParams& p = working;
	switch (f)
	{
	case FieldId::VALUE:
		p.value = clamp_val(p.value, delta, 0, 99999);
		break;
	case FieldId::BASE_WEIGHT:
		p.baseWeight = clamp_val(p.baseWeight, delta, 0, 100);
		break;
	case FieldId::WEIGHT:
		p.weight = clamp_val(p.weight, delta, 0, 1000);
		break;
	case FieldId::LEVEL_MIN:
		p.levelMin = clamp_val(p.levelMin, delta, 1, 20);
		break;
	case FieldId::LEVEL_MAX:
		p.levelMax = clamp_val(p.levelMax, delta, 0, 20);
		break;
	case FieldId::LEVEL_SCALING:
	{
		float raw = p.levelScaling + static_cast<float>(delta) * 0.01f;
		p.levelScaling = std::clamp(raw, 0.0f, 5.0f);
		break;
	}
	case FieldId::CONSUMABLE_AMT:
		p.consumableAmount = clamp_val(p.consumableAmount, delta, 0, 9999);
		break;
	case FieldId::DURATION:
		p.duration = clamp_val(p.duration, delta, 0, 9999);
		break;
	case FieldId::RANGE:
		p.range = clamp_val(p.range, delta, 0, 50);
		break;
	case FieldId::DAMAGE:
		p.damage = clamp_val(p.damage, delta, 0, 999);
		break;
	case FieldId::CONFUSE_TURNS:
		p.confuseTurns = clamp_val(p.confuseTurns, delta, 0, 999);
		break;
	case FieldId::STRENGTH_RATING:
	{
		// 0 is an ordinary weapon; a rating is a Strength score, and Table 1 stops at 25.
		p.strengthRating = clamp_val(p.strengthRating, delta, 0, 25);
		break;
	}
	case FieldId::AC_BONUS:
		p.acBonus = clamp_val(p.acBonus, delta, -10, 10);
		break;
	case FieldId::EFFECT_BONUS:
		p.effectBonus = clamp_val(p.effectBonus, delta, 0, 20);
		break;
	case FieldId::STR_BONUS:
		p.strBonus = clamp_val(p.strBonus, delta, -18, 18);
		break;
	case FieldId::DEX_BONUS:
		p.dexBonus = clamp_val(p.dexBonus, delta, -18, 18);
		break;
	case FieldId::CON_BONUS:
		p.conBonus = clamp_val(p.conBonus, delta, -18, 18);
		break;
	case FieldId::INT_BONUS:
		p.intBonus = clamp_val(p.intBonus, delta, -18, 18);
		break;
	case FieldId::WIS_BONUS:
		p.wisBonus = clamp_val(p.wisBonus, delta, -18, 18);
		break;
	case FieldId::CHA_BONUS:
		p.chaBonus = clamp_val(p.chaBonus, delta, -18, 18);
		break;
	case FieldId::EXCEPTIONAL_STRENGTH:
	{
		p.exceptionalStrength = clamp_val(p.exceptionalStrength, delta, 0, 100);
		break;
	}
	case FieldId::NUTRITION:
		p.nutritionValue = clamp_val(p.nutritionValue, delta, 0, 9999);
		break;
	default:
		break;
	}
}

void ItemEditor::field_toggle(FieldId f)
{
	ItemParams& p = working;
	switch (f)
	{
	case FieldId::COLOR:
		p.color = next_color_pair(p.color);
		break;
	case FieldId::ITEM_CLASS:
		p.itemClass = next_item_class(p.itemClass);
		break;
	case FieldId::PICKABLE_TYPE:
		p.pickableType = next_pickable_type(p.pickableType);
		break;
	case FieldId::CONSUMABLE_EFFECT:
		p.consumableEffect = next_consumable_effect(p.consumableEffect);
		break;
	case FieldId::CONSUMABLE_BUFF:
		p.consumableBuffType = next_buff_type(p.consumableBuffType);
		break;
	case FieldId::TARGET_MODE:
		p.targetMode = next_target_mode(p.targetMode);
		break;
	case FieldId::SCROLL_ANIM:
		p.scrollAnimation = next_scroll_animation(p.scrollAnimation);
		break;
	case FieldId::RANGED:
		p.ranged = !p.ranged;
		break;
	case FieldId::HAND_REQUIREMENT:
		p.handRequirement = next_hand_requirement(p.handRequirement);
		break;
	case FieldId::WEAPON_SIZE:
		p.weaponSize = next_weapon_size(p.weaponSize);
		break;
	case FieldId::EFFECT:
		p.effect = next_magical_effect(p.effect);
		break;
	case FieldId::IS_SET_MODE:
		p.isSetMode = !p.isSetMode;
		break;
	default:
		break;
	}
}

void ItemEditor::field_set_string(FieldId f, std::string val)
{
	switch (f)
	{
	case FieldId::NAME:
		workingName = std::move(val);
		break;
	case FieldId::CATEGORY:
		workingCategory = std::move(val);
		break;
	default:
		break;
	}
}

// end of file: ItemEditor.cpp
