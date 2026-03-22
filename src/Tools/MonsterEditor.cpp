// file: MonsterEditor.cpp
#include <algorithm>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Factories/MonsterCreator.h"
#include "../Menu/Menu.h"
#include "../Renderer/Renderer.h"
#include "MonsterEditor.h"

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

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MonsterEditor::enter()
{
	m_active = true;
	m_mode = Mode::NORMAL;
	m_focus = 0;

	m_keys = MonsterCreator::get_all_keys();

	m_list_cursor = 0;
	m_list_scroll = 0;
	m_picker_sheet = 0;
	m_picker_scroll = 0;

	load_working();
}

void MonsterEditor::exit(GameContext& ctx)
{
	commit_working();
	m_active = false;
	ctx.menus->push_back(std::make_unique<Menu>(true, ctx));
}

void MonsterEditor::tick(GameContext& ctx)
{
	handle_input(ctx);

	ctx.renderer->begin_frame();
	render(ctx);
	ctx.renderer->end_frame();
}

// ---------------------------------------------------------------------------
// Working copy
// ---------------------------------------------------------------------------

void MonsterEditor::load_working()
{
	if (m_keys.empty())
		return;

	const std::string& key = current_key();
	m_is_class_based = MonsterCreator::is_class_key(key);
	m_working = MonsterCreator::get_params(key);
	m_field_cursor = m_is_class_based ? static_cast<int>(FieldId::TILE) : 0;
	m_field_scroll = 0;
	m_mode = Mode::NORMAL;
	m_edit_buf.clear();
}

void MonsterEditor::commit_working()
{
	if (m_keys.empty())
		return;
	MonsterCreator::set_params(current_key(), m_working);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void MonsterEditor::handle_input(GameContext& ctx)
{
	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

	if (IsKeyPressed(KEY_ESCAPE))
	{
		exit(ctx);
		return;
	}

	if (ctrl && IsKeyPressed(KEY_S))
	{
		commit_working();
		MonsterCreator::save(Paths::MONSTERS);
		m_last_save_time = GetTime();
		return;
	}

	if (IsKeyPressed(KEY_TAB))
	{
		m_focus = 1 - m_focus;
		return;
	}

	if (m_mode == Mode::NORMAL)
	{
		handle_normal(ctx);
	}
	else if (m_mode == Mode::EDIT_STRING)
	{
		handle_edit_string();
	}
	else if (m_mode == Mode::TILE_PICKER)
	{
		handle_picker(*ctx.renderer);
	}
}

void MonsterEditor::handle_normal(const GameContext& ctx)
{
	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	const Renderer& r = *ctx.renderer;
	int sh = r.get_screen_height();
	int body_h = sh - HEADER_H - HINT_H;
	int visible_fields = body_h / FIELD_H;
	int total = static_cast<int>(m_keys.size());

	::Vector2 mouse = GetMousePosition();
	bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

	// Mouse: click selects panel and item
	if (clicked)
	{
		if (mouse.x < LIST_W)
		{
			m_focus = 0;
			constexpr int ITEM_H = 30;
			int idx = m_list_scroll + static_cast<int>(mouse.y - HEADER_H) / ITEM_H;
			idx = std::clamp(idx, 0, total - 1);
			if (idx != m_list_cursor)
			{
				commit_working();
				m_list_cursor = idx;
				load_working();
			}
		}
		else if (m_mode == Mode::NORMAL)
		{
			m_focus = 1;
			int max_field = m_is_class_based ? static_cast<int>(FieldId::TILE) : FIELD_COUNT - 1;
			int idx = m_field_scroll + static_cast<int>(mouse.y - HEADER_H) / FIELD_H;
			idx = std::clamp(idx, 0, max_field);
			m_field_cursor = m_is_class_based ? static_cast<int>(FieldId::TILE) : idx;
		}
	}

	// Mouse wheel scrolling
	float wheel = GetMouseWheelMove();
	if (wheel != 0.0f)
	{
		if (mouse.x < LIST_W)
		{
			m_list_scroll = std::clamp(
				m_list_scroll - static_cast<int>(wheel),
				0,
				std::max(0, total - 1));
		}
		else if (!m_is_class_based)
		{
			m_field_scroll = std::clamp(
				m_field_scroll - static_cast<int>(wheel),
				0,
				std::max(0, FIELD_COUNT - visible_fields));
		}
	}

	// Keyboard: list panel
	if (m_focus == 0)
	{
		if (IsKeyPressed(KEY_UP) && m_list_cursor > 0)
		{
			commit_working();
			--m_list_cursor;
			if (m_list_cursor < m_list_scroll)
			{
				m_list_scroll = m_list_cursor;
			}
			load_working();
		}
		else if (IsKeyPressed(KEY_DOWN) && m_list_cursor < total - 1)
		{
			commit_working();
			++m_list_cursor;
			int vis_list = body_h / 30;
			if (m_list_cursor >= m_list_scroll + vis_list)
			{
				m_list_scroll = m_list_cursor - vis_list + 1;
			}
			load_working();
		}
		else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_RIGHT))
		{
			m_focus = 1;
		}
		else if (IsKeyPressed(KEY_A))
		{
			commit_working();
			MonsterParams defaults;
			defaults.name = "New Monster";
			defaults.corpse_name = "dead monster";
			defaults.hp_dice = DiceExpr{ 1, 8, 0 };
			defaults.thaco = 20;
			defaults.ac = 10;
			defaults.xp = 1;
			defaults.str_dice = DiceExpr{ 3, 6, 0 };
			defaults.dex_dice = DiceExpr{ 3, 6, 0 };
			defaults.con_dice = DiceExpr{ 3, 6, 0 };
			defaults.int_dice = DiceExpr{ 3, 6, 0 };
			defaults.wis_dice = DiceExpr{ 3, 6, 0 };
			defaults.cha_dice = DiceExpr{ 3, 6, 0 };
			defaults.weapon_name = "claws";
			defaults.damage = DamageInfo{ 1, 4, "1d4", DamageType::PHYSICAL };
			defaults.base_weight = 10;
			defaults.level_minimum = 1;
			defaults.level_maximum = 5;
			std::string new_key = MonsterCreator::add_custom(std::move(defaults));
			m_keys = MonsterCreator::get_all_keys();
			for (int i = 0; i < static_cast<int>(m_keys.size()); ++i)
			{
				if (m_keys[i] == new_key)
				{
					m_list_cursor = i;
					break;
				}
			}
			load_working();
			m_focus = 1;
		}
		else if (IsKeyPressed(KEY_DELETE))
		{
			if (!MonsterCreator::is_builtin(current_key()) && !MonsterCreator::is_class_key(current_key()))
			{
				MonsterCreator::remove_custom(current_key());
				m_keys = MonsterCreator::get_all_keys();
				m_list_cursor = std::clamp(m_list_cursor, 0, static_cast<int>(m_keys.size()) - 1);
				load_working();
			}
		}
		return;
	}

	// Keyboard: field panel
	if (m_is_class_based)
	{
		// Only TILE field is editable for class-based monsters
		if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_F2))
		{
			m_mode = Mode::TILE_PICKER;
		}
		return;
	}

	int field_max = FIELD_COUNT - 1;

	if (IsKeyPressed(KEY_UP) && m_field_cursor > 0)
	{
		--m_field_cursor;
		if (m_field_cursor < m_field_scroll)
		{
			m_field_scroll = m_field_cursor;
		}
	}
	else if (IsKeyPressed(KEY_DOWN) && m_field_cursor < field_max)
	{
		++m_field_cursor;
		if (m_field_cursor >= m_field_scroll + visible_fields)
		{
			m_field_scroll = m_field_cursor - visible_fields + 1;
		}
	}
	else if (IsKeyPressed(KEY_LEFT) && m_focus == 0)
	{
		m_focus = 0;
	}

	FieldId fid = current_field();

	if (IsKeyPressed(KEY_F2) || (fid == FieldId::TILE && IsKeyPressed(KEY_ENTER)))
	{
		m_mode = Mode::TILE_PICKER;
		return;
	}

	if (IsKeyPressed(KEY_ENTER))
	{
		if (field_is_string(fid))
		{
			m_edit_buf = field_value(fid);
			m_mode = Mode::EDIT_STRING;
		}
		else if (field_is_toggle(fid))
		{
			field_toggle(fid);
		}
		return;
	}

	// Left/Right for int fields
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

void MonsterEditor::handle_edit_string()
{
	int ch = GetCharPressed();
	while (ch != 0)
	{
		if (ch >= 32 && ch < 127)
		{
			m_edit_buf += static_cast<char>(ch);
		}
		ch = GetCharPressed();
	}

	if (IsKeyPressed(KEY_BACKSPACE) && !m_edit_buf.empty())
	{
		m_edit_buf.pop_back();
	}

	if (IsKeyPressed(KEY_ENTER))
	{
		field_set_string(current_field(), m_edit_buf);
		m_mode = Mode::NORMAL;
	}

	if (IsKeyPressed(KEY_ESCAPE))
	{
		m_mode = Mode::NORMAL;
	}
}

void MonsterEditor::handle_picker(const Renderer& r)
{
	int total_sheets = r.get_loaded_sheet_count();

	auto advance_sheet = [&](int dir)
	{
		for (int i = 0; i < total_sheets; ++i)
		{
			m_picker_sheet = (m_picker_sheet + dir + total_sheets) % total_sheets;
			if (r.sheet_is_loaded(static_cast<TileSheet>(m_picker_sheet)))
			{
				break;
			}
		}
		m_picker_scroll = 0;
	};

	if (!r.sheet_is_loaded(static_cast<TileSheet>(m_picker_sheet)))
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
		m_mode = Mode::NORMAL;
		return;
	}

	int sheet_rows = r.get_sheet_rows(static_cast<TileSheet>(m_picker_sheet));
	::Vector2 mouse = GetMousePosition();
	bool in_picker = mouse.x >= LIST_W;

	if (in_picker)
	{
		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			m_picker_scroll = std::clamp(
				m_picker_scroll - static_cast<int>(wheel),
				0,
				std::max(0, sheet_rows - 1));
		}
	}

	constexpr int PICKER_TILE = 36;
	constexpr int PAD = 10;
	constexpr int SUB_HDR_H = 28;
	int grid_y = HEADER_H + SUB_HDR_H;

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && in_picker)
	{
		int col = static_cast<int>(mouse.x - LIST_W - PAD) / (PICKER_TILE + 2);
		int row = m_picker_scroll + static_cast<int>(mouse.y - grid_y) / (PICKER_TILE + 2);
		int sheet_cols = r.get_sheet_cols(static_cast<TileSheet>(m_picker_sheet));

		if (col >= 0 && col < sheet_cols && row >= 0 && row < sheet_rows)
		{
			m_working.symbol = TileRef{ static_cast<TileSheet>(m_picker_sheet), col, row };
			m_mode = Mode::NORMAL;
		}
	}
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void MonsterEditor::render(const GameContext& ctx) const
{
	const Renderer& r = *ctx.renderer;
	int sw = r.get_screen_width();
	int sh = r.get_screen_height();

	DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 255 });

	render_header(r);
	render_list(r);

	if (m_mode == Mode::TILE_PICKER)
	{
		render_picker(r);
	}
	else
	{
		render_fields(r);
	}

	render_hint(r);
}

void MonsterEditor::render_header(const Renderer& r) const
{
	int sw = r.get_screen_width();
	DrawRectangle(0, 0, sw, HEADER_H, Color{ 20, 20, 40, 255 });
	r.draw_text_color(8, 6, "MONSTER EDITOR", Color{ 255, 255, 180, 255 });
	r.draw_text_color(8, 26,
		"Tab:switch focus  Left/Right:adjust  Enter:edit  F2:tile  Ctrl+S:save  Esc:exit",
		Color{ 130, 130, 100, 255 });
}

void MonsterEditor::render_list(const Renderer& r) const
{
	int sh = r.get_screen_height();
	int body_y = HEADER_H;
	int body_h = sh - HEADER_H - HINT_H;
	constexpr int ITEM_H = 30;
	constexpr int TILE_SZ = 22;
	constexpr int PAD = 6;

	DrawRectangle(0, body_y, LIST_W, body_h, Color{ 10, 10, 20, 255 });
	DrawLine(LIST_W, body_y, LIST_W, body_y + body_h, Color{ 80, 80, 120, 255 });

	int total = static_cast<int>(m_keys.size());
	int vis = body_h / ITEM_H;
	int max_scroll = std::max(0, total - vis);
	int scroll = std::clamp(m_list_scroll, 0, max_scroll);

	::Vector2 mouse = GetMousePosition();

	for (int i = scroll; i < total; ++i)
	{
		int iy = body_y + (i - scroll) * ITEM_H;
		if (iy + ITEM_H > body_y + body_h)
			break;

		// Section separators
		if (i > scroll)
		{
			bool prev_builtin = MonsterCreator::is_builtin(m_keys[i - 1]);
			bool prev_class = MonsterCreator::is_class_key(m_keys[i - 1]);
			bool cur_class = MonsterCreator::is_class_key(m_keys[i]);
			bool cur_custom = !MonsterCreator::is_builtin(m_keys[i]) && !cur_class;
			bool prev_custom = !prev_builtin && !prev_class;

			if ((prev_builtin && cur_custom) || (prev_custom && cur_class))
				DrawLine(PAD, iy, LIST_W - PAD, iy, Color{ 70, 70, 70, 200 });
		}

		bool is_sel = (i == m_list_cursor);
		bool hovered = mouse.x >= 0 && mouse.x < LIST_W
			&& mouse.y >= iy && mouse.y < iy + ITEM_H;

		Color bg{ 0, 0, 0, 0 };
		if (is_sel && m_focus == 0)
			bg = Color{ 60, 60, 0, 200 };
		else if (is_sel)
			bg = Color{ 40, 40, 0, 150 };
		else if (hovered)
			bg = Color{ 30, 30, 30, 160 };

		if (bg.a > 0)
			DrawRectangle(0, iy, LIST_W, ITEM_H, bg);

		TileRef tile = is_sel ? m_working.symbol : MonsterCreator::get_tile(m_keys[i]);
		r.draw_tile_screen_sized(PAD, iy + (ITEM_H - TILE_SZ) / 2, tile, TILE_SZ);

		bool is_class = MonsterCreator::is_class_key(m_keys[i]);
		bool is_custom = !MonsterCreator::is_builtin(m_keys[i]) && !is_class;

		std::string display_name = is_sel
			? m_working.name
			: MonsterCreator::get_params(m_keys[i]).name;
		if (display_name.empty())
			display_name = prettify_key(m_keys[i]);
		if (is_custom)
			display_name += " *";

		Color tc;
		if (is_sel)
			tc = is_class ? Color{ 255, 200, 80, 255 } : Color{ 255, 255, 100, 255 };
		else
			tc = is_class ? Color{ 150, 130, 110, 255 } : Color{ 200, 200, 200, 255 };

		r.draw_text_color(PAD + TILE_SZ + 4, iy + (ITEM_H - 16) / 2, display_name, tc);
	}
}

void MonsterEditor::render_fields(const Renderer& r) const
{
	int sw = r.get_screen_width();
	int sh = r.get_screen_height();
	int ox = LIST_W;
	int oy = HEADER_H;
	int fw = sw - LIST_W;
	int fh = sh - HEADER_H - HINT_H;

	DrawRectangle(ox, oy, fw, fh, Color{ 8, 8, 16, 255 });

	if (m_is_class_based)
	{
		r.draw_text_color(ox + 12, oy + 10,
			"Class-based monster -- only tile is editable.",
			Color{ 160, 160, 100, 255 });

		constexpr int TILE_SZ = 40;
		int ty = oy + 44;
		DrawRectangle(ox, ty, fw, FIELD_H, Color{ 60, 60, 0, 200 });
		r.draw_text_color(ox + 12, ty + (FIELD_H - 16) / 2, "Tile", Color{ 255, 220, 80, 255 });
		r.draw_tile_screen_sized(sw - TILE_SZ - 12, ty + (FIELD_H - TILE_SZ) / 2, m_working.symbol, TILE_SZ);
		DrawRectangleLines(sw - TILE_SZ - 12, ty + (FIELD_H - TILE_SZ) / 2, TILE_SZ, TILE_SZ, Color{ 255, 255, 0, 255 });
		r.draw_text_color(ox + 12, ty + FIELD_H + 8,
			"Press Enter or F2 to open tile picker.",
			Color{ 130, 130, 100, 255 });
		return;
	}

	int visible = fh / FIELD_H;
	int max_scroll = std::max(0, FIELD_COUNT - visible);
	int scroll = std::clamp(m_field_scroll, 0, max_scroll);

	::Vector2 mouse = GetMousePosition();

	for (int i = scroll; i < FIELD_COUNT; ++i)
	{
		int iy = oy + (i - scroll) * FIELD_H;
		if (iy + FIELD_H > oy + fh)
		{
			break;
		}

		FieldId fid = static_cast<FieldId>(i);
		bool is_sel = (i == m_field_cursor);
		bool hovered = mouse.x >= ox && mouse.x < sw
			&& mouse.y >= iy && mouse.y < iy + FIELD_H;

		Color bg{ 0, 0, 0, 0 };
		if (is_sel && m_focus == 1)
		{
			bg = Color{ 60, 60, 0, 200 };
		}
		else if (is_sel)
		{
			bg = Color{ 40, 40, 0, 140 };
		}
		else if (hovered)
		{
			bg = Color{ 20, 20, 10, 120 };
		}

		if (bg.a > 0)
		{
			DrawRectangle(ox, iy, fw, FIELD_H, bg);
		}

		Color lc = is_sel ? Color{ 255, 220, 80, 255 } : Color{ 150, 150, 150, 255 };
		Color vc = is_sel ? Color{ 255, 255, 200, 255 } : Color{ 200, 200, 200, 255 };

		r.draw_text_color(ox + 12, iy + (FIELD_H - 16) / 2, field_label(fid), lc);

		if (fid == FieldId::TILE)
		{
			constexpr int TILE_SZ = 22;
			r.draw_tile_screen_sized(sw - TILE_SZ - 12, iy + (FIELD_H - TILE_SZ) / 2, m_working.symbol, TILE_SZ);
			if (is_sel)
			{
				DrawRectangleLines(sw - TILE_SZ - 12, iy + (FIELD_H - TILE_SZ) / 2, TILE_SZ, TILE_SZ, Color{ 255, 255, 0, 255 });
			}
		}
		else
		{
			std::string val;
			if (is_sel && m_mode == Mode::EDIT_STRING)
			{
				val = m_edit_buf + "_";
			}
			else
			{
				val = field_value(fid);
			}
			int vx = sw - r.measure_text(val) - 16;
			r.draw_text_color(vx, iy + (FIELD_H - 16) / 2, val, vc);
		}
	}
}

void MonsterEditor::render_picker(const Renderer& r) const
{
	int sw = r.get_screen_width();
	int sh = r.get_screen_height();
	int ox = LIST_W;
	int ow = sw - LIST_W;
	int body_y = HEADER_H;
	int body_h = sh - HEADER_H - HINT_H;
	constexpr int SUB_HDR_H = 28;
	constexpr int PICKER_TILE = 36;
	constexpr int PAD = 10;

	DrawRectangle(ox, body_y, ow, body_h, Color{ 8, 8, 24, 255 });

	DrawRectangle(ox, body_y, ow, SUB_HDR_H, Color{ 15, 15, 40, 255 });
	int total_sheets = r.get_loaded_sheet_count();
	std::string hdr = std::format(
		"Sheet: {} ({}/{})  --  Left/Right:change  Esc/F2:back",
		r.get_sheet_name(static_cast<TileSheet>(m_picker_sheet)),
		m_picker_sheet + 1,
		total_sheets);
	r.draw_text_color(ox + PAD, body_y + 6, hdr, Color{ 200, 200, 120, 255 });

	int grid_y = body_y + SUB_HDR_H;
	int grid_h = body_h - SUB_HDR_H;
	int sheet_cols = r.get_sheet_cols(static_cast<TileSheet>(m_picker_sheet));
	int sheet_rows = r.get_sheet_rows(static_cast<TileSheet>(m_picker_sheet));

	::Vector2 mouse = GetMousePosition();

	BeginScissorMode(ox, grid_y, ow, grid_h);

	for (int row = m_picker_scroll; row < sheet_rows; ++row)
	{
		int py = grid_y + (row - m_picker_scroll) * (PICKER_TILE + 2);
		if (py >= grid_y + grid_h)
		{
			break;
		}

		for (int col = 0; col < sheet_cols; ++col)
		{
			int px = ox + PAD + col * (PICKER_TILE + 2);
			TileRef tid{ static_cast<TileSheet>(m_picker_sheet), col, row };

			bool is_cur = (tid == m_working.symbol);
			bool hovered = mouse.x >= px && mouse.x < px + PICKER_TILE
				&& mouse.y >= py && mouse.y < py + PICKER_TILE;

			if (is_cur)
			{
				DrawRectangle(px, py, PICKER_TILE, PICKER_TILE, Color{ 60, 60, 0, 220 });
			}
			else if (hovered)
			{
				DrawRectangle(px, py, PICKER_TILE, PICKER_TILE, Color{ 40, 40, 20, 160 });
			}

			r.draw_tile_screen_sized(px, py, tid, PICKER_TILE);

			if (is_cur)
			{
				DrawRectangleLines(px, py, PICKER_TILE, PICKER_TILE, Color{ 255, 255, 0, 255 });
			}
		}
	}

	EndScissorMode();
}

void MonsterEditor::render_hint(const Renderer& r) const
{
	int sw = r.get_screen_width();
	int sh = r.get_screen_height();
	int hint_y = sh - HINT_H;

	DrawRectangle(0, hint_y, sw, HINT_H, Color{ 20, 20, 40, 255 });

	std::string msg;
	bool saved_flash = (GetTime() - m_last_save_time) < 2.0;

	if (saved_flash)
	{
		msg = "Saved!";
	}
	else if (m_mode == Mode::EDIT_STRING)
	{
		msg = "Typing  --  Enter:confirm  Esc:cancel";
	}
	else if (m_mode == Mode::TILE_PICKER)
	{
		msg = "Tile Picker  --  Left/Right:sheet  Click:assign  Esc/F2:back";
	}
	else if (m_focus == 0)
	{
		msg = "[LIST] Up/Down:navigate  A:add new  Del:remove custom  Enter:edit fields  Tab:switch  Ctrl+S:save  Esc:exit";
	}
	else
	{
		msg = "[FIELDS] Up/Down:navigate  Left/Right:adjust  Enter:edit  F2:tile  Tab:switch  Ctrl+S:save  Esc:exit";
	}

	Color hc = saved_flash ? Color{ 100, 255, 100, 255 } : Color{ 160, 160, 120, 255 };
	r.draw_text_color(8, hint_y + 6, msg, hc);
}

// ---------------------------------------------------------------------------
// Field accessors
// ---------------------------------------------------------------------------

const std::string& MonsterEditor::current_key() const
{
	if (m_keys.empty())
		throw std::out_of_range("MonsterEditor::current_key -- key list is empty");
	return m_keys[m_list_cursor];
}

MonsterEditor::FieldId MonsterEditor::current_field() const
{
	if (m_is_class_based)
	{
		return FieldId::TILE;
	}
	return static_cast<FieldId>(m_field_cursor);
}

std::string MonsterEditor::field_label(FieldId f) const
{
	switch (f)
	{
	case FieldId::NAME:       return "Name";
	case FieldId::CORPSE:     return "Corpse";
	case FieldId::HP_NUM:     return "HP Num";
	case FieldId::HP_SIDES:   return "HP Sides";
	case FieldId::HP_BONUS:   return "HP Bonus";
	case FieldId::THACO:      return "THAC0";
	case FieldId::AC:         return "AC";
	case FieldId::XP:         return "XP";
	case FieldId::DR:         return "DR";
	case FieldId::STR_NUM:    return "STR Num";
	case FieldId::STR_SIDES:  return "STR Sides";
	case FieldId::STR_BONUS:  return "STR Bonus";
	case FieldId::DEX_NUM:    return "DEX Num";
	case FieldId::DEX_SIDES:  return "DEX Sides";
	case FieldId::DEX_BONUS:  return "DEX Bonus";
	case FieldId::CON_NUM:    return "CON Num";
	case FieldId::CON_SIDES:  return "CON Sides";
	case FieldId::CON_BONUS:  return "CON Bonus";
	case FieldId::INT_NUM:    return "INT Num";
	case FieldId::INT_SIDES:  return "INT Sides";
	case FieldId::INT_BONUS:  return "INT Bonus";
	case FieldId::WIS_NUM:    return "WIS Num";
	case FieldId::WIS_SIDES:  return "WIS Sides";
	case FieldId::WIS_BONUS:  return "WIS Bonus";
	case FieldId::CHA_NUM:    return "CHA Num";
	case FieldId::CHA_SIDES:  return "CHA Sides";
	case FieldId::CHA_BONUS:  return "CHA Bonus";
	case FieldId::WEAPON:     return "Weapon";
	case FieldId::DMG_MIN:    return "Dmg Min";
	case FieldId::DMG_MAX:    return "Dmg Max";
	case FieldId::DMG_DISPLAY: return "Dmg Display";
	case FieldId::AI_TYPE:    return "AI Type";
	case FieldId::CAN_SWIM:   return "Can Swim";
	case FieldId::WEIGHT:     return "Spawn Weight";
	case FieldId::DEPTH_MIN:  return "Depth Min";
	case FieldId::DEPTH_MAX:  return "Depth Max";
	case FieldId::TILE:       return "Tile";
	default:                  return "???";
	}
}

std::string MonsterEditor::field_value(FieldId f) const
{
	switch (f)
	{
	case FieldId::NAME:        return m_working.name;
	case FieldId::CORPSE:      return m_working.corpse_name;
	case FieldId::HP_NUM:      return std::format("{}", m_working.hp_dice.num);
	case FieldId::HP_SIDES:    return std::format("{}", m_working.hp_dice.sides);
	case FieldId::HP_BONUS:    return std::format("{}", m_working.hp_dice.bonus);
	case FieldId::THACO:       return std::format("{}", m_working.thaco);
	case FieldId::AC:          return std::format("{}", m_working.ac);
	case FieldId::XP:          return std::format("{}", m_working.xp);
	case FieldId::DR:          return std::format("{}", m_working.dr);
	case FieldId::STR_NUM:     return std::format("{}", m_working.str_dice.num);
	case FieldId::STR_SIDES:   return std::format("{}", m_working.str_dice.sides);
	case FieldId::STR_BONUS:   return std::format("{}", m_working.str_dice.bonus);
	case FieldId::DEX_NUM:     return std::format("{}", m_working.dex_dice.num);
	case FieldId::DEX_SIDES:   return std::format("{}", m_working.dex_dice.sides);
	case FieldId::DEX_BONUS:   return std::format("{}", m_working.dex_dice.bonus);
	case FieldId::CON_NUM:     return std::format("{}", m_working.con_dice.num);
	case FieldId::CON_SIDES:   return std::format("{}", m_working.con_dice.sides);
	case FieldId::CON_BONUS:   return std::format("{}", m_working.con_dice.bonus);
	case FieldId::INT_NUM:     return std::format("{}", m_working.int_dice.num);
	case FieldId::INT_SIDES:   return std::format("{}", m_working.int_dice.sides);
	case FieldId::INT_BONUS:   return std::format("{}", m_working.int_dice.bonus);
	case FieldId::WIS_NUM:     return std::format("{}", m_working.wis_dice.num);
	case FieldId::WIS_SIDES:   return std::format("{}", m_working.wis_dice.sides);
	case FieldId::WIS_BONUS:   return std::format("{}", m_working.wis_dice.bonus);
	case FieldId::CHA_NUM:     return std::format("{}", m_working.cha_dice.num);
	case FieldId::CHA_SIDES:   return std::format("{}", m_working.cha_dice.sides);
	case FieldId::CHA_BONUS:   return std::format("{}", m_working.cha_dice.bonus);
	case FieldId::WEAPON:      return m_working.weapon_name;
	case FieldId::DMG_MIN:     return std::format("{}", m_working.damage.minDamage);
	case FieldId::DMG_MAX:     return std::format("{}", m_working.damage.maxDamage);
	case FieldId::DMG_DISPLAY: return m_working.damage.displayRoll;
	case FieldId::AI_TYPE:     return m_working.ai_type == MonsterAiType::MELEE ? "melee" : "ranged";
	case FieldId::CAN_SWIM:    return m_working.can_swim ? "yes" : "no";
	case FieldId::WEIGHT:      return std::format("{}", m_working.base_weight);
	case FieldId::DEPTH_MIN:   return std::format("{}", m_working.level_minimum);
	case FieldId::DEPTH_MAX:   return std::format("{}", m_working.level_maximum);
	case FieldId::TILE:        return "(tile)";
	default:                   return "";
	}
}

bool MonsterEditor::field_is_string(FieldId f) const
{
	return f == FieldId::NAME
		|| f == FieldId::CORPSE
		|| f == FieldId::WEAPON
		|| f == FieldId::DMG_DISPLAY;
}

bool MonsterEditor::field_is_toggle(FieldId f) const
{
	return f == FieldId::AI_TYPE || f == FieldId::CAN_SWIM;
}

void MonsterEditor::field_adjust(FieldId f, int delta)
{
	auto clamp_val = [](int v, int d, int lo, int hi) -> int
	{
		return std::clamp(v + d, lo, hi);
	};

	switch (f)
	{
	case FieldId::HP_NUM:     m_working.hp_dice.num    = clamp_val(m_working.hp_dice.num,    delta, 1,   99); break;
	case FieldId::HP_SIDES:   m_working.hp_dice.sides  = clamp_val(m_working.hp_dice.sides,  delta, 2,   20); break;
	case FieldId::HP_BONUS:   m_working.hp_dice.bonus  = clamp_val(m_working.hp_dice.bonus,  delta, -99, 99); break;
	case FieldId::THACO:      m_working.thaco           = clamp_val(m_working.thaco,           delta, 1,   25); break;
	case FieldId::AC:         m_working.ac              = clamp_val(m_working.ac,              delta, -10, 10); break;
	case FieldId::XP:         m_working.xp              = clamp_val(m_working.xp,              delta, 0,   9999); break;
	case FieldId::DR:         m_working.dr              = clamp_val(m_working.dr,              delta, 0,   20); break;
	case FieldId::STR_NUM:    m_working.str_dice.num    = clamp_val(m_working.str_dice.num,    delta, 1,   10); break;
	case FieldId::STR_SIDES:  m_working.str_dice.sides  = clamp_val(m_working.str_dice.sides,  delta, 2,   20); break;
	case FieldId::STR_BONUS:  m_working.str_dice.bonus  = clamp_val(m_working.str_dice.bonus,  delta, -18, 18); break;
	case FieldId::DEX_NUM:    m_working.dex_dice.num    = clamp_val(m_working.dex_dice.num,    delta, 1,   10); break;
	case FieldId::DEX_SIDES:  m_working.dex_dice.sides  = clamp_val(m_working.dex_dice.sides,  delta, 2,   20); break;
	case FieldId::DEX_BONUS:  m_working.dex_dice.bonus  = clamp_val(m_working.dex_dice.bonus,  delta, -18, 18); break;
	case FieldId::CON_NUM:    m_working.con_dice.num    = clamp_val(m_working.con_dice.num,    delta, 1,   10); break;
	case FieldId::CON_SIDES:  m_working.con_dice.sides  = clamp_val(m_working.con_dice.sides,  delta, 2,   20); break;
	case FieldId::CON_BONUS:  m_working.con_dice.bonus  = clamp_val(m_working.con_dice.bonus,  delta, -18, 18); break;
	case FieldId::INT_NUM:    m_working.int_dice.num    = clamp_val(m_working.int_dice.num,    delta, 1,   10); break;
	case FieldId::INT_SIDES:  m_working.int_dice.sides  = clamp_val(m_working.int_dice.sides,  delta, 2,   20); break;
	case FieldId::INT_BONUS:  m_working.int_dice.bonus  = clamp_val(m_working.int_dice.bonus,  delta, -18, 18); break;
	case FieldId::WIS_NUM:    m_working.wis_dice.num    = clamp_val(m_working.wis_dice.num,    delta, 1,   10); break;
	case FieldId::WIS_SIDES:  m_working.wis_dice.sides  = clamp_val(m_working.wis_dice.sides,  delta, 2,   20); break;
	case FieldId::WIS_BONUS:  m_working.wis_dice.bonus  = clamp_val(m_working.wis_dice.bonus,  delta, -18, 18); break;
	case FieldId::CHA_NUM:    m_working.cha_dice.num    = clamp_val(m_working.cha_dice.num,    delta, 1,   10); break;
	case FieldId::CHA_SIDES:  m_working.cha_dice.sides  = clamp_val(m_working.cha_dice.sides,  delta, 2,   20); break;
	case FieldId::CHA_BONUS:  m_working.cha_dice.bonus  = clamp_val(m_working.cha_dice.bonus,  delta, -18, 18); break;
	case FieldId::DMG_MIN:    m_working.damage.minDamage = clamp_val(m_working.damage.minDamage, delta, 1, 99); break;
	case FieldId::DMG_MAX:    m_working.damage.maxDamage = clamp_val(m_working.damage.maxDamage, delta, 1, 99); break;
	case FieldId::WEIGHT:     m_working.base_weight      = clamp_val(m_working.base_weight,      delta, 0, 100); break;
	case FieldId::DEPTH_MIN:  m_working.level_minimum    = clamp_val(m_working.level_minimum,    delta, 1, 20); break;
	case FieldId::DEPTH_MAX:  m_working.level_maximum    = clamp_val(m_working.level_maximum,    delta, 0, 20); break;
	default: break;
	}
}

void MonsterEditor::field_toggle(FieldId f)
{
	if (f == FieldId::AI_TYPE)
	{
		m_working.ai_type = (m_working.ai_type == MonsterAiType::MELEE)
			? MonsterAiType::RANGED
			: MonsterAiType::MELEE;
	}
	else if (f == FieldId::CAN_SWIM)
	{
		m_working.can_swim = !m_working.can_swim;
	}
}

void MonsterEditor::field_set_string(FieldId f, std::string val)
{
	switch (f)
	{
	case FieldId::NAME:        m_working.name = std::move(val); break;
	case FieldId::CORPSE:      m_working.corpse_name = std::move(val); break;
	case FieldId::WEAPON:      m_working.weapon_name = std::move(val); break;
	case FieldId::DMG_DISPLAY: m_working.damage.displayRoll = std::move(val); break;
	default: break;
	}
}

// end of file: MonsterEditor.cpp
