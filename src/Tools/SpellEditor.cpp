// file: SpellEditor.cpp
#include <algorithm>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Menu/Menu.h"
#include "../Renderer/Renderer.h"
#include "../Systems/SpellSystem.h"
#include "SpellEditor.h"

constexpr int LIST_WIDTH = 220;
constexpr int HEADER_HEIGHT = 48;
constexpr int HINT_HEIGHT = 28;
constexpr int FIELD_HEIGHT = 30;
constexpr int ITEM_HEIGHT = 32;
constexpr int LIST_PAD = 8;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void SpellEditor::enter()
{
	m_active = true;
	m_mode = Mode::NORMAL;
	m_focus = 0;

	m_keys = SpellSystem::get_all_keys();

	m_list_cursor = 0;
	m_list_scroll = 0;

	load_working();
}

void SpellEditor::exit(GameContext& ctx)
{
	commit_working();
	m_active = false;
	ctx.menus->push_back(make_main_menu(true, ctx));
}

void SpellEditor::tick(GameContext& ctx)
{
	handle_input(ctx);

	ctx.renderer->begin_frame();
	render(ctx);
	ctx.renderer->end_frame();
}

// ---------------------------------------------------------------------------
// Working copy
// ---------------------------------------------------------------------------

void SpellEditor::load_working()
{
	if (m_keys.empty())
		return;
	m_working = SpellSystem::get_by_key(current_key());
	m_field_cursor = 0;
	m_mode = Mode::NORMAL;
	m_edit_buf.clear();
}

void SpellEditor::commit_working()
{
	if (m_keys.empty())
		return;
	SpellSystem::set_by_key(current_key(), m_working);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void SpellEditor::handle_input(GameContext& ctx)
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
		SpellSystem::save(Paths::SPELLS);
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
}

void SpellEditor::handle_normal(const GameContext& ctx)
{
	int total = static_cast<int>(m_keys.size());
	::Vector2 mouse = GetMousePosition();
	bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

	if (clicked)
	{
		if (mouse.x < LIST_WIDTH)
		{
			m_focus = 0;
			int idx = m_list_scroll + static_cast<int>(mouse.y - HEADER_HEIGHT) / ITEM_HEIGHT;
			idx = std::clamp(idx, 0, total - 1);
			if (idx != m_list_cursor)
			{
				commit_working();
				m_list_cursor = idx;
				load_working();
			}
		}
		else
		{
			m_focus = 1;
			int idx = static_cast<int>(mouse.y - HEADER_HEIGHT) / FIELD_HEIGHT;
			idx = std::clamp(idx, 0, FIELD_COUNT - 1);
			m_field_cursor = idx;
		}
	}

	// Mouse wheel
	float wheel = GetMouseWheelMove();
	if (wheel != 0.0f && mouse.x < LIST_WIDTH)
	{
		m_list_scroll = std::clamp(
			m_list_scroll - static_cast<int>(wheel),
			0,
			std::max(0, total - 1));
	}

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
			int screenHeight = ctx.renderer->get_screen_height();
			int visibleCount = (screenHeight - HEADER_HEIGHT - HINT_HEIGHT) / 32;
			if (m_list_cursor >= m_list_scroll + visibleCount)
			{
				m_list_scroll = m_list_cursor - visibleCount + 1;
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
			SpellDefinition def;
			def.name = "New Spell";
			def.level = 1;
			def.spellClass = SpellClass::WIZARD;
			def.description = "No effect yet.";
			std::string new_key = SpellSystem::add_custom(std::move(def));
			m_keys = SpellSystem::get_all_keys();
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
			if (!SpellSystem::is_builtin_key(current_key()))
			{
				SpellSystem::remove_custom(current_key());
				m_keys = SpellSystem::get_all_keys();
				m_list_cursor = std::clamp(m_list_cursor, 0, static_cast<int>(m_keys.size()) - 1);
				load_working();
			}
		}
		return;
	}

	// Field panel navigation
	if (IsKeyPressed(KEY_UP) && m_field_cursor > 0)
	{
		--m_field_cursor;
	}
	else if (IsKeyPressed(KEY_DOWN) && m_field_cursor < FIELD_COUNT - 1)
	{
		++m_field_cursor;
	}

	FieldId fid = current_field();

	if (IsKeyPressed(KEY_ENTER))
	{
		if (field_is_string(fid))
		{
			m_edit_buf = field_value(fid);
			m_mode = Mode::EDIT_STRING;
		}
		else
		{
			field_cycle(fid);
		}
		return;
	}

	if (!field_is_string(fid))
	{
		if (IsKeyPressed(KEY_LEFT))
		{
			field_adjust(fid, -1);
		}
		else if (IsKeyPressed(KEY_RIGHT))
		{
			field_adjust(fid, 1);
		}
	}
}

void SpellEditor::handle_edit_string()
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

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void SpellEditor::render(const GameContext& ctx) const
{
	const Renderer& r = *ctx.renderer;
	int screenWidth = r.get_screen_width();
	int screenHeight = r.get_screen_height();

	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 255 });

	render_header(r);
	render_list(r);
	render_fields(r);
	render_hint(r);
}

void SpellEditor::render_header(const Renderer& r) const
{
	int screenWidth = r.get_screen_width();
	DrawRectangle(0, 0, screenWidth, HEADER_HEIGHT, Color{ 20, 20, 40, 255 });
	r.draw_text_color(Vector2D{ 8, 6 }, "SPELL EDITOR", Color{ 180, 255, 180, 255 });
	r.draw_text_color(Vector2D{ 8, 26 },
		"Tab:switch focus  Up/Down:navigate  Left/Right:adjust  Enter:edit/cycle  Ctrl+S:save  Esc:exit",
		Color{ 130, 130, 100, 255 });
}

void SpellEditor::render_list(const Renderer& r) const
{
	int screenHeight = r.get_screen_height();
	int body_y = HEADER_HEIGHT;
	int body_h = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;

	DrawRectangle(0, body_y, LIST_WIDTH, body_h, Color{ 10, 10, 20, 255 });
	DrawLine(LIST_WIDTH, body_y, LIST_WIDTH, body_y + body_h, Color{ 80, 80, 120, 255 });

	int total = static_cast<int>(m_keys.size());
	int visibleCount = body_h / ITEM_HEIGHT;
	int scroll = std::clamp(m_list_scroll, 0, std::max(0, total - visibleCount));

	::Vector2 mouse = GetMousePosition();

	for (int i = scroll; i < total; ++i)
	{
		int itemY = body_y + (i - scroll) * ITEM_HEIGHT;
		if (itemY + ITEM_HEIGHT > body_y + body_h)
		{
			break;
		}

		bool is_sel = (i == m_list_cursor);
		bool hovered = mouse.x >= 0 && mouse.x < LIST_WIDTH
			&& mouse.y >= itemY && mouse.y < itemY + ITEM_HEIGHT;

		Color bgColor{ 0, 0, 0, 0 };
		if (is_sel && m_focus == 0)
		{
			bgColor = Color{ 0, 60, 0, 200 };
		}
		else if (is_sel)
		{
			bgColor = Color{ 0, 40, 0, 150 };
		}
		else if (hovered)
		{
			bgColor = Color{ 30, 30, 30, 160 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(0, itemY, LIST_WIDTH, ITEM_HEIGHT, bgColor);
		}

		// Show spell class indicator
		const SpellDefinition& def = is_sel
			? m_working
			: SpellSystem::get_by_key(m_keys[i]);

		bool is_custom = !SpellSystem::is_builtin_key(m_keys[i]);

		Color class_col = (def.spellClass == SpellClass::CLERIC)
			? Color{ 100, 220, 255, 255 }
			: Color{ 255, 160, 80, 255 };

		std::string label = std::format("[{}] {}{}", def.level, def.name, is_custom ? " *" : "");
		Color textColor = is_sel ? Color{ 180, 255, 180, 255 } : Color{ 200, 200, 200, 255 };

		r.draw_text_color(Vector2D{ LIST_PAD, itemY + (ITEM_HEIGHT - 16) / 2 }, label, textColor);

		// Class dot on right edge
		const char* class_tag = (def.spellClass == SpellClass::CLERIC) ? "C" : "W";
		r.draw_text_color(Vector2D{ LIST_WIDTH - 20, itemY + (ITEM_HEIGHT - 16) / 2 }, class_tag, class_col);
	}
}

void SpellEditor::render_fields(const Renderer& r) const
{
	int screenWidth = r.get_screen_width();
	int screenHeight = r.get_screen_height();
	int panelX = LIST_WIDTH;
	int panelY = HEADER_HEIGHT;
	int fieldsWidth = screenWidth - LIST_WIDTH;
	int fieldsHeight = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;

	DrawRectangle(panelX, panelY, fieldsWidth, fieldsHeight, Color{ 8, 8, 16, 255 });

	::Vector2 mouse = GetMousePosition();

	for (int i = 0; i < FIELD_COUNT; ++i)
	{
		int itemY = panelY + i * FIELD_HEIGHT;
		if (itemY + FIELD_HEIGHT > panelY + fieldsHeight)
		{
			break;
		}

		FieldId fid = static_cast<FieldId>(i);
		bool is_sel = (i == m_field_cursor);
		bool hovered = mouse.x >= panelX && mouse.x < screenWidth
			&& mouse.y >= itemY && mouse.y < itemY + FIELD_HEIGHT;

		Color bgColor{ 0, 0, 0, 0 };
		if (is_sel && m_focus == 1)
		{
			bgColor = Color{ 0, 60, 0, 200 };
		}
		else if (is_sel)
		{
			bgColor = Color{ 0, 40, 0, 140 };
		}
		else if (hovered)
		{
			bgColor = Color{ 20, 30, 20, 120 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(panelX, itemY, fieldsWidth, FIELD_HEIGHT, bgColor);
		}

		Color labelColor = is_sel ? Color{ 180, 255, 180, 255 } : Color{ 150, 150, 150, 255 };
		Color valueColor = is_sel ? Color{ 255, 255, 200, 255 } : Color{ 200, 200, 200, 255 };

		r.draw_text_color(Vector2D{ panelX + 12, itemY + (FIELD_HEIGHT - 16) / 2 }, field_label(fid), labelColor);

		std::string val;
		if (is_sel && m_mode == Mode::EDIT_STRING)
		{
			val = m_edit_buf + "_";
		}
		else
		{
			val = field_value(fid);
		}

		int vx = screenWidth - r.measure_text(val) - 16;
		r.draw_text_color(Vector2D{ vx, itemY + (FIELD_HEIGHT - 16) / 2 }, val, valueColor);
	}
}

void SpellEditor::render_hint(const Renderer& r) const
{
	int screenWidth = r.get_screen_width();
	int screenHeight = r.get_screen_height();
	int hint_y = screenHeight - HINT_HEIGHT;

	DrawRectangle(0, hint_y, screenWidth, HINT_HEIGHT, Color{ 20, 20, 40, 255 });

	bool saved_flash = (GetTime() - m_last_save_time) < 2.0;
	std::string msg;

	if (saved_flash)
	{
		msg = "Saved!";
	}
	else if (m_mode == Mode::EDIT_STRING)
	{
		msg = "Typing  --  Enter:confirm  Esc:cancel";
	}
	else if (m_focus == 0)
	{
		msg = "[LIST] Up/Down:navigate  A:add new  Del:remove custom  Enter:edit fields  Tab:switch  Ctrl+S:save  Esc:exit";
	}
	else
	{
		msg = "[FIELDS] Up/Down:navigate  Left/Right:adjust  Enter:edit/cycle  Tab:switch  Ctrl+S:save  Esc:exit";
	}

	Color hintColor = saved_flash ? Color{ 100, 255, 100, 255 } : Color{ 160, 160, 120, 255 };
	r.draw_text_color(Vector2D{ 8, hint_y + 6 }, msg, hintColor);
}

// ---------------------------------------------------------------------------
// Field accessors
// ---------------------------------------------------------------------------

const std::string& SpellEditor::current_key() const
{
	if (m_keys.empty())
		throw std::out_of_range("SpellEditor::current_key -- key list is empty");
	return m_keys[m_list_cursor];
}

SpellEditor::FieldId SpellEditor::current_field() const
{
	return static_cast<FieldId>(m_field_cursor);
}

namespace
{

std::string effect_type_name(SpellEffectType e)
{
	switch (e)
	{
	case SpellEffectType::CURE_LIGHT_WOUNDS: return "Cure Light Wounds";
	case SpellEffectType::BLESS:             return "Bless";
	case SpellEffectType::SANCTUARY:         return "Sanctuary";
	case SpellEffectType::HOLD_PERSON:       return "Hold Person";
	case SpellEffectType::SILENCE:           return "Silence";
	case SpellEffectType::MAGIC_MISSILE:     return "Magic Missile";
	case SpellEffectType::SHIELD:            return "Shield";
	case SpellEffectType::SLEEP:             return "Sleep";
	case SpellEffectType::INVISIBILITY:      return "Invisibility";
	case SpellEffectType::WEB:               return "Web";
	case SpellEffectType::FIREBALL:          return "Fireball";
	case SpellEffectType::TELEPORT:          return "Teleport";
	default:                                 return "None";
	}
}

SpellEffectType effect_type_next(SpellEffectType e)
{
	int v = static_cast<int>(e) + 1;
	constexpr int LAST = static_cast<int>(SpellEffectType::NONE);
	if (v > LAST) v = 0;
	return static_cast<SpellEffectType>(v);
}

SpellEffectType effect_type_prev(SpellEffectType e)
{
	int v = static_cast<int>(e) - 1;
	constexpr int LAST = static_cast<int>(SpellEffectType::NONE);
	if (v < 0) v = LAST;
	return static_cast<SpellEffectType>(v);
}

} // namespace

std::string SpellEditor::field_label(FieldId f) const
{
	switch (f)
	{
	case FieldId::NAME:        return "Name";
	case FieldId::LEVEL:       return "Level";
	case FieldId::CLASS:       return "Class";
	case FieldId::EFFECT_TYPE: return "Effect";
	case FieldId::DESCRIPTION: return "Description";
	default:                   return "???";
	}
}

std::string SpellEditor::field_value(FieldId f) const
{
	switch (f)
	{
	case FieldId::NAME:        return m_working.name;
	case FieldId::LEVEL:       return std::format("{}", m_working.level);
	case FieldId::CLASS:
		if (m_working.spellClass == SpellClass::CLERIC) return "cleric";
		if (m_working.spellClass == SpellClass::WIZARD) return "wizard";
		return "both";
	case FieldId::EFFECT_TYPE: return effect_type_name(m_working.effect_type);
	case FieldId::DESCRIPTION: return m_working.description;
	default:                   return "";
	}
}

bool SpellEditor::field_is_string(FieldId f) const
{
	return f == FieldId::NAME || f == FieldId::DESCRIPTION;
}

void SpellEditor::field_adjust(FieldId f, int delta)
{
	if (f == FieldId::LEVEL)
	{
		m_working.level = std::clamp(m_working.level + delta, 1, 9);
	}
	else if (f == FieldId::EFFECT_TYPE)
	{
		if (delta > 0)
			m_working.effect_type = effect_type_next(m_working.effect_type);
		else
			m_working.effect_type = effect_type_prev(m_working.effect_type);
	}
}

void SpellEditor::field_cycle(FieldId f)
{
	if (f == FieldId::CLASS)
	{
		if (m_working.spellClass == SpellClass::CLERIC)
		{
			m_working.spellClass = SpellClass::WIZARD;
		}
		else if (m_working.spellClass == SpellClass::WIZARD)
		{
			m_working.spellClass = SpellClass::BOTH;
		}
		else
		{
			m_working.spellClass = SpellClass::CLERIC;
		}
	}
	else if (f == FieldId::EFFECT_TYPE)
	{
		m_working.effect_type = effect_type_next(m_working.effect_type);
	}
}

void SpellEditor::field_set_string(FieldId f, std::string val)
{
	if (f == FieldId::NAME)
	{
		m_working.name = std::move(val);
	}
	else if (f == FieldId::DESCRIPTION)
	{
		m_working.description = std::move(val);
	}
}

// end of file: SpellEditor.cpp
