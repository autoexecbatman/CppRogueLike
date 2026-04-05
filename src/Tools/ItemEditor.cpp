// file: ItemEditor.cpp
#include <algorithm>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

#include "../Core/GameContext.h"
#include "../Core/Paths.h"
#include "../Factories/ItemCreator.h"
#include "../Items/ItemClassification.h"
#include "../Items/MagicalItemEffects.h"
#include "../Items/Weapons.h"
#include "../Menu/Menu.h"
#include "../Renderer/Renderer.h"
#include "../Systems/BuffType.h"
#include "../Systems/ContentRegistry.h"
#include "../Systems/ContentRegistryIO.h"
#include "../Systems/TargetMode.h"
#include "ItemEditor.h"

constexpr int LIST_WIDTH = 220;
constexpr int HEADER_HEIGHT = 48;
constexpr int HINT_HEIGHT = 28;
constexpr int FIELD_HEIGHT = 26;
constexpr int ITEM_HEIGHT = 30;
constexpr int LIST_TILE_SIZE = 22;
constexpr int LIST_PAD = 6;
constexpr int PICKER_PAD = 10;
constexpr int PICKER_SUB_HEADER_HEIGHT = 28;
constexpr int PICKER_TILE_SIZE = 36;

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

template <typename E>
E cycle_enum(E val, int count)
{
	int v = (static_cast<int>(val) + 1) % count;
	return static_cast<E>(v);
}

std::string_view item_class_str(ItemClass c)
{
	switch (c)
	{
	case ItemClass::UNKNOWN:    return "unknown";
	case ItemClass::DAGGER:     return "dagger";
	case ItemClass::SWORD:      return "sword";
	case ItemClass::GREAT_SWORD: return "great_sword";
	case ItemClass::AXE:        return "axe";
	case ItemClass::HAMMER:     return "hammer";
	case ItemClass::MACE:       return "mace";
	case ItemClass::STAFF:      return "staff";
	case ItemClass::BOW:        return "bow";
	case ItemClass::CROSSBOW:   return "crossbow";
	case ItemClass::ARMOR:      return "armor";
	case ItemClass::SHIELD:     return "shield";
	case ItemClass::HELMET:     return "helmet";
	case ItemClass::RING:       return "ring";
	case ItemClass::AMULET:     return "amulet";
	case ItemClass::GAUNTLETS:  return "gauntlets";
	case ItemClass::GIRDLE:     return "girdle";
	case ItemClass::POTION:     return "potion";
	case ItemClass::SCROLL:     return "scroll";
	case ItemClass::FOOD:       return "food";
	case ItemClass::GOLD_COIN:  return "gold_coin";
	case ItemClass::GEM:        return "gem";
	case ItemClass::TOOL:       return "tool";
	case ItemClass::QUEST_ITEM: return "quest_item";
	}
	return "unknown";
}

std::string_view pickable_type_str(PickableType t)
{
	switch (t)
	{
	case PickableType::TARGETED_SCROLL: return "targeted_scroll";
	case PickableType::TELEPORTER:      return "teleporter";
	case PickableType::WEAPON:          return "weapon";
	case PickableType::SHIELD:          return "shield";
	case PickableType::CONSUMABLE:      return "consumable";
	case PickableType::GOLD_COIN:       return "gold_coin";
	case PickableType::FOOD:            return "food";
	case PickableType::CORPSE_FOOD:     return "corpse_food";
	case PickableType::ARMOR:           return "armor";
	case PickableType::MAGICAL_HELM:    return "magical_helm";
	case PickableType::MAGICAL_RING:    return "magical_ring";
	case PickableType::JEWELRY_AMULET:  return "jewelry_amulet";
	case PickableType::GAUNTLETS:       return "gauntlets";
	case PickableType::GIRDLE:          return "girdle";
	case PickableType::QUEST_ITEM:      return "quest_item";
	}
	return "weapon";
}

std::string_view consumable_effect_str(ConsumableEffect e)
{
	switch (e)
	{
	case ConsumableEffect::NONE:     return "none";
	case ConsumableEffect::HEAL:     return "heal";
	case ConsumableEffect::ADD_BUFF: return "add_buff";
	case ConsumableEffect::FAIL:     return "fail";
	}
	return "none";
}

std::string_view buff_type_str(BuffType b)
{
	switch (b)
	{
	case BuffType::NONE:                 return "none";
	case BuffType::INVISIBILITY:         return "invisibility";
	case BuffType::BLESS:                return "bless";
	case BuffType::SHIELD:               return "shield";
	case BuffType::STRENGTH:             return "strength";
	case BuffType::DEXTERITY:            return "dexterity";
	case BuffType::CONSTITUTION:         return "constitution";
	case BuffType::INTELLIGENCE:         return "intelligence";
	case BuffType::WISDOM:               return "wisdom";
	case BuffType::CHARISMA:             return "charisma";
	case BuffType::SPEED:                return "speed";
	case BuffType::FIRE_RESISTANCE:      return "fire_resistance";
	case BuffType::COLD_RESISTANCE:      return "cold_resistance";
	case BuffType::LIGHTNING_RESISTANCE: return "lightning_resistance";
	case BuffType::POISON_RESISTANCE:    return "poison_resistance";
	case BuffType::SLEEP:                return "sleep";
	case BuffType::HOLD_PERSON:          return "hold_person";
	case BuffType::SANCTUARY:            return "sanctuary";
	case BuffType::SILENCE:              return "silence";
	case BuffType::WEBBED:               return "webbed";
	}
	return "none";
}

std::string_view target_mode_str(TargetMode m)
{
	switch (m)
	{
	case TargetMode::AUTO_NEAREST:     return "auto_nearest";
	case TargetMode::PICK_TILE_SINGLE: return "pick_tile_single";
	case TargetMode::PICK_TILE_AOE:    return "pick_tile_aoe";
	case TargetMode::FOV_BUFF:         return "fov_buff";
	}
	return "auto_nearest";
}

std::string_view scroll_anim_str(ScrollAnimation a)
{
	switch (a)
	{
	case ScrollAnimation::NONE:      return "none";
	case ScrollAnimation::LIGHTNING: return "lightning";
	case ScrollAnimation::EXPLOSION: return "explosion";
	}
	return "none";
}

std::string_view hand_req_str(HandRequirement h)
{
	switch (h)
	{
	case HandRequirement::ONE_HANDED:    return "one_handed";
	case HandRequirement::TWO_HANDED:    return "two_handed";
	case HandRequirement::OFF_HAND_ONLY: return "off_hand_only";
	}
	return "one_handed";
}

std::string_view weapon_size_str(WeaponSize s)
{
	switch (s)
	{
	case WeaponSize::TINY:   return "tiny";
	case WeaponSize::SMALL:  return "small";
	case WeaponSize::MEDIUM: return "medium";
	case WeaponSize::LARGE:  return "large";
	case WeaponSize::GIANT:  return "giant";
	}
	return "medium";
}

std::string_view magical_effect_str(MagicalEffect e)
{
	switch (e)
	{
	case MagicalEffect::NONE:              return "none";
	case MagicalEffect::BRILLIANCE:        return "brilliance";
	case MagicalEffect::TELEPORTATION:     return "teleportation";
	case MagicalEffect::TELEPATHY:         return "telepathy";
	case MagicalEffect::UNDERWATER_ACTION: return "underwater_action";
	case MagicalEffect::FREE_ACTION:       return "free_action";
	case MagicalEffect::REGENERATION:      return "regeneration";
	case MagicalEffect::INVISIBILITY:      return "invisibility";
	case MagicalEffect::FIRE_RESISTANCE:   return "fire_resistance";
	case MagicalEffect::COLD_RESISTANCE:   return "cold_resistance";
	case MagicalEffect::SPELL_STORING:     return "spell_storing";
	case MagicalEffect::PROTECTION:        return "protection";
	}
	return "none";
}

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ItemEditor::enter(GameContext& ctx)
{
	m_registry = ctx.contentRegistry;
	m_active = true;
	m_mode = Mode::NORMAL;
	m_focus = 0;

	m_keys = ItemCreator::get_all_keys();

	m_list_cursor = 0;
	m_list_scroll = 0;
	m_picker_sheet = 0;
	m_picker_scroll = 0;

	load_working();
}

void ItemEditor::exit(GameContext& ctx)
{
	commit_working();
	m_active = false;
	ctx.menus->push_back(std::make_unique<Menu>(true, ctx));
}

void ItemEditor::tick(GameContext& ctx)
{
	handle_input(ctx);

	ctx.renderer->begin_frame();
	render(ctx);
	ctx.renderer->end_frame();
}

// ---------------------------------------------------------------------------
// Working copy
// ---------------------------------------------------------------------------

void ItemEditor::load_working()
{
	if (m_keys.empty())
		return;

	const std::string& key = current_key();
	const ItemParams& p = ItemCreator::get_params(key);
	m_working = p;
	m_working_name = std::string{ p.name };
	m_working_category = std::string{ p.category };
	m_working_tile = m_registry ? m_registry->get_tile(key) : TileRef{};
	m_field_cursor = 0;
	m_field_scroll = 0;
	m_mode = Mode::NORMAL;
	m_edit_buf.clear();
}

void ItemEditor::commit_working()
{
	if (m_keys.empty())
		return;
	const std::string& key = current_key();
	ItemCreator::set_name_category(key, m_working_name, m_working_category);
	ItemCreator::set_params(key, m_working);
	if (m_registry)
	{
		m_registry->set_tile(key, m_working_tile);
	}
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void ItemEditor::handle_input(GameContext& ctx)
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
		ItemCreator::save(Paths::ITEMS);
		if (m_registry)
		{
			ContentRegistryIO::save(*m_registry, Paths::CONTENT_TILES);
		}
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

void ItemEditor::handle_normal(const GameContext& ctx)
{
	bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
	const Renderer& r = *ctx.renderer;
	int screenHeight = r.get_screen_height();
	int body_h = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;
	int visible_fields = body_h / FIELD_HEIGHT;
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
		else if (m_mode == Mode::NORMAL)
		{
			m_focus = 1;
			int idx = m_field_scroll + static_cast<int>(mouse.y - HEADER_HEIGHT) / FIELD_HEIGHT;
			idx = std::clamp(idx, 0, FIELD_COUNT - 1);
			m_field_cursor = idx;
		}
	}

	float wheel = GetMouseWheelMove();
	if (wheel != 0.0f)
	{
		if (mouse.x < LIST_WIDTH)
		{
			m_list_scroll = std::clamp(
				m_list_scroll - static_cast<int>(wheel),
				0,
				std::max(0, total - 1));
		}
		else
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
				m_list_scroll = m_list_cursor;
			load_working();
		}
		else if (IsKeyPressed(KEY_DOWN) && m_list_cursor < total - 1)
		{
			commit_working();
			++m_list_cursor;
			int vis_list = body_h / 30;
			if (m_list_cursor >= m_list_scroll + vis_list)
				m_list_scroll = m_list_cursor - vis_list + 1;
			load_working();
		}
		else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_RIGHT))
		{
			m_focus = 1;
		}
		else if (IsKeyPressed(KEY_A))
		{
			commit_working();
			ItemParams defaults;
			defaults.itemClass = ItemClass::SWORD;
			defaults.pickableType = PickableType::WEAPON;
			defaults.value = 10;
			defaults.baseWeight = 10;
			defaults.levelMin = 1;
			defaults.levelMax = 5;
			defaults.handRequirement = HandRequirement::ONE_HANDED;
			defaults.weaponSize = WeaponSize::MEDIUM;
			std::string new_key = ItemCreator::add_custom("New Item", "weapon", defaults);
			m_keys = ItemCreator::get_all_keys();
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
			if (!ItemCreator::is_builtin_key(current_key()))
			{
				ItemCreator::remove_custom(current_key());
				m_keys = ItemCreator::get_all_keys();
				m_list_cursor = std::clamp(m_list_cursor, 0, static_cast<int>(m_keys.size()) - 1);
				load_working();
			}
		}
		return;
	}

	// Keyboard: field panel
	int field_max = FIELD_COUNT - 1;

	if (IsKeyPressed(KEY_UP) && m_field_cursor > 0)
	{
		--m_field_cursor;
		if (m_field_cursor < m_field_scroll)
			m_field_scroll = m_field_cursor;
	}
	else if (IsKeyPressed(KEY_DOWN) && m_field_cursor < field_max)
	{
		++m_field_cursor;
		if (m_field_cursor >= m_field_scroll + visible_fields)
			m_field_scroll = m_field_cursor - visible_fields + 1;
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
			m_edit_buf += static_cast<char>(ch);
		ch = GetCharPressed();
	}

	if (IsKeyPressed(KEY_BACKSPACE) && !m_edit_buf.empty())
		m_edit_buf.pop_back();

	if (IsKeyPressed(KEY_ENTER))
	{
		field_set_string(current_field(), m_edit_buf);
		m_mode = Mode::NORMAL;
	}

	if (IsKeyPressed(KEY_ESCAPE))
		m_mode = Mode::NORMAL;
}

void ItemEditor::handle_picker(const Renderer& r)
{
	int total_sheets = r.get_loaded_sheet_count();

	auto advance_sheet = [&](int dir)
	{
		for (int i = 0; i < total_sheets; ++i)
		{
			m_picker_sheet = (m_picker_sheet + dir + total_sheets) % total_sheets;
			if (r.sheet_is_loaded(static_cast<TileSheet>(m_picker_sheet)))
				break;
		}
		m_picker_scroll = 0;
	};

	if (!r.sheet_is_loaded(static_cast<TileSheet>(m_picker_sheet)))
		advance_sheet(1);

	if (IsKeyPressed(KEY_LEFT))
		advance_sheet(-1);
	if (IsKeyPressed(KEY_RIGHT))
		advance_sheet(1);
	if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F2))
	{
		m_mode = Mode::NORMAL;
		return;
	}

	int sheet_rows = r.get_sheet_rows(static_cast<TileSheet>(m_picker_sheet));
	::Vector2 mouse = GetMousePosition();
	bool in_picker = mouse.x >= LIST_WIDTH;

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

	int grid_y = HEADER_HEIGHT + PICKER_SUB_HEADER_HEIGHT;

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && in_picker)
	{
		int col = static_cast<int>(mouse.x - LIST_WIDTH - PICKER_PAD) / (PICKER_TILE_SIZE + 2);
		int row = m_picker_scroll + static_cast<int>(mouse.y - grid_y) / (PICKER_TILE_SIZE + 2);
		int sheet_cols = r.get_sheet_cols(static_cast<TileSheet>(m_picker_sheet));

		if (col >= 0 && col < sheet_cols && row >= 0 && row < sheet_rows)
		{
			m_working_tile = TileRef{ static_cast<TileSheet>(m_picker_sheet), col, row };
			m_mode = Mode::NORMAL;
		}
	}
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void ItemEditor::render(const GameContext& ctx) const
{
	const Renderer& r = *ctx.renderer;
	int screenWidth = r.get_screen_width();
	int screenHeight = r.get_screen_height();

	DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 255 });

	render_header(r);
	render_list(r);

	if (m_mode == Mode::TILE_PICKER)
		render_picker(r);
	else
		render_fields(r);

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

void ItemEditor::render_list(const Renderer& r) const
{
	int screenHeight = r.get_screen_height();
	int body_y = HEADER_HEIGHT;
	int body_h = screenHeight - HEADER_HEIGHT - HINT_HEIGHT;

	DrawRectangle(0, body_y, LIST_WIDTH, body_h, Color{ 5, 10, 5, 255 });
	DrawLine(LIST_WIDTH, body_y, LIST_WIDTH, body_y + body_h, Color{ 80, 120, 80, 255 });

	int total = static_cast<int>(m_keys.size());
	int visibleCount = body_h / ITEM_HEIGHT;
	int max_scroll = std::max(0, total - visibleCount);
	int scroll = std::clamp(m_list_scroll, 0, max_scroll);

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
			bgColor = Color{ 20, 30, 20, 160 };
		}

		if (bgColor.a > 0)
		{
			DrawRectangle(0, itemY, LIST_WIDTH, ITEM_HEIGHT, bgColor);
		}

		TileRef tile = is_sel
			? m_working_tile
			: (m_registry ? m_registry->get_tile(m_keys[i]) : TileRef{});
		r.draw_tile_screen_sized(Vector2D{ LIST_PAD, itemY + (ITEM_HEIGHT - LIST_TILE_SIZE) / 2 }, tile, LIST_TILE_SIZE);

		bool is_custom = !ItemCreator::is_builtin_key(m_keys[i]);

		std::string display_name = is_sel
			? m_working_name
			: std::string{ ItemCreator::get_params(m_keys[i]).name };

		if (display_name.empty())
		{
			display_name = prettify_key(m_keys[i]);
		}

		if (is_custom)
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
	int scroll = std::clamp(m_field_scroll, 0, max_scroll);

	::Vector2 mouse = GetMousePosition();

	for (int i = scroll; i < FIELD_COUNT; ++i)
	{
		int itemY = panelY + (i - scroll) * FIELD_HEIGHT;
		if (itemY + FIELD_HEIGHT > panelY + fieldsHeight)
			break;

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
					r.draw_tile_screen_sized(Vector2D{ screenWidth - LIST_TILE_SIZE - 12, itemY + (FIELD_HEIGHT - LIST_TILE_SIZE) / 2 }, m_working_tile, LIST_TILE_SIZE);
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
		r.get_sheet_name(static_cast<TileSheet>(m_picker_sheet)),
		m_picker_sheet + 1,
		total_sheets);
	r.draw_text_color(Vector2D{ panelX + PICKER_PAD, body_y + 6 }, hdr, Color{ 120, 200, 200, 255 });

	int grid_y = body_y + PICKER_SUB_HEADER_HEIGHT;
	int grid_h = body_h - PICKER_SUB_HEADER_HEIGHT;
	int sheet_cols = r.get_sheet_cols(static_cast<TileSheet>(m_picker_sheet));
	int sheet_rows = r.get_sheet_rows(static_cast<TileSheet>(m_picker_sheet));

	::Vector2 mouse = GetMousePosition();

	BeginScissorMode(panelX, grid_y, panelWidth, grid_h);

	for (int row = m_picker_scroll; row < sheet_rows; ++row)
	{
		int py = grid_y + (row - m_picker_scroll) * (PICKER_TILE_SIZE + 2);
		if (py >= grid_y + grid_h)
			break;

		for (int col = 0; col < sheet_cols; ++col)
		{
			int px = panelX + PICKER_PAD + col * (PICKER_TILE_SIZE + 2);
			TileRef tid{ static_cast<TileSheet>(m_picker_sheet), col, row };

			bool is_cur = (tid == m_working_tile);
			bool hovered = mouse.x >= px && mouse.x < px + PICKER_TILE_SIZE
				&& mouse.y >= py && mouse.y < py + PICKER_TILE_SIZE;

			if (is_cur)
				DrawRectangle(px, py, PICKER_TILE_SIZE, PICKER_TILE_SIZE, Color{ 0, 60, 0, 220 });
			else if (hovered)
				DrawRectangle(px, py, PICKER_TILE_SIZE, PICKER_TILE_SIZE, Color{ 20, 40, 20, 160 });

			r.draw_tile_screen_sized(Vector2D{ px, py }, tid, PICKER_TILE_SIZE);

			if (is_cur)
				DrawRectangleLines(px, py, PICKER_TILE_SIZE, PICKER_TILE_SIZE, Color{ 0, 255, 100, 255 });
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
	if (m_keys.empty())
		throw std::out_of_range("ItemEditor::current_key -- key list is empty");
	return m_keys[m_list_cursor];
}

ItemEditor::FieldId ItemEditor::current_field() const
{
	return static_cast<FieldId>(m_field_cursor);
}

std::string ItemEditor::field_label(FieldId f) const
{
	switch (f)
	{
	case FieldId::NAME:             return "Name";
	case FieldId::CATEGORY:         return "Category";
	case FieldId::ITEM_CLASS:       return "Item Class";
	case FieldId::PICKABLE_TYPE:    return "Pickable Type";
	case FieldId::COLOR:            return "Color";
	case FieldId::VALUE:            return "Value";
	case FieldId::BASE_WEIGHT:      return "Base Weight";
	case FieldId::LEVEL_MIN:        return "Level Min";
	case FieldId::LEVEL_MAX:        return "Level Max";
	case FieldId::LEVEL_SCALING:    return "Level Scaling";
	case FieldId::CONSUMABLE_EFFECT: return "Consumable Effect";
	case FieldId::CONSUMABLE_BUFF:  return "Consumable Buff";
	case FieldId::CONSUMABLE_AMT:   return "Consumable Amount";
	case FieldId::DURATION:         return "Duration";
	case FieldId::TARGET_MODE:      return "Target Mode";
	case FieldId::SCROLL_ANIM:      return "Scroll Animation";
	case FieldId::RANGE:            return "Range";
	case FieldId::DAMAGE:           return "Damage";
	case FieldId::CONFUSE_TURNS:    return "Confuse Turns";
	case FieldId::RANGED:           return "Ranged";
	case FieldId::HAND_REQUIREMENT: return "Hand Requirement";
	case FieldId::WEAPON_SIZE:      return "Weapon Size";
	case FieldId::AC_BONUS:         return "AC Bonus";
	case FieldId::EFFECT:           return "Effect";
	case FieldId::EFFECT_BONUS:     return "Effect Bonus";
	case FieldId::STR_BONUS:        return "STR Bonus";
	case FieldId::DEX_BONUS:        return "DEX Bonus";
	case FieldId::CON_BONUS:        return "CON Bonus";
	case FieldId::INT_BONUS:        return "INT Bonus";
	case FieldId::WIS_BONUS:        return "WIS Bonus";
	case FieldId::CHA_BONUS:        return "CHA Bonus";
	case FieldId::IS_SET_MODE:      return "Is Set Mode";
	case FieldId::NUTRITION:        return "Nutrition Value";
	case FieldId::GOLD_AMOUNT:      return "Gold Amount";
	case FieldId::TILE:             return "Tile";
	default:                        return "???";
	}
}

std::string ItemEditor::field_value(FieldId f) const
{
	const ItemParams& p = m_working;
	switch (f)
	{
	case FieldId::NAME:             return m_working_name;
	case FieldId::CATEGORY:         return m_working_category;
	case FieldId::ITEM_CLASS:       return std::string{ item_class_str(p.itemClass) };
	case FieldId::PICKABLE_TYPE:    return std::string{ pickable_type_str(p.pickableType) };
	case FieldId::COLOR:            return std::format("{}", p.color);
	case FieldId::VALUE:            return std::format("{}", p.value);
	case FieldId::BASE_WEIGHT:      return std::format("{}", p.baseWeight);
	case FieldId::LEVEL_MIN:        return std::format("{}", p.levelMin);
	case FieldId::LEVEL_MAX:        return std::format("{}", p.levelMax);
	case FieldId::LEVEL_SCALING:    return std::format("{:.2f}", p.levelScaling);
	case FieldId::CONSUMABLE_EFFECT: return std::string{ consumable_effect_str(p.consumableEffect) };
	case FieldId::CONSUMABLE_BUFF:  return std::string{ buff_type_str(p.consumableBuffType) };
	case FieldId::CONSUMABLE_AMT:   return std::format("{}", p.consumableAmount);
	case FieldId::DURATION:         return std::format("{}", p.duration);
	case FieldId::TARGET_MODE:      return std::string{ target_mode_str(p.targetMode) };
	case FieldId::SCROLL_ANIM:      return std::string{ scroll_anim_str(p.scrollAnimation) };
	case FieldId::RANGE:            return std::format("{}", p.range);
	case FieldId::DAMAGE:           return std::format("{}", p.damage);
	case FieldId::CONFUSE_TURNS:    return std::format("{}", p.confuseTurns);
	case FieldId::RANGED:           return p.ranged ? "yes" : "no";
	case FieldId::HAND_REQUIREMENT: return std::string{ hand_req_str(p.handRequirement) };
	case FieldId::WEAPON_SIZE:      return std::string{ weapon_size_str(p.weaponSize) };
	case FieldId::AC_BONUS:         return std::format("{}", p.acBonus);
	case FieldId::EFFECT:           return std::string{ magical_effect_str(p.effect) };
	case FieldId::EFFECT_BONUS:     return std::format("{}", p.effectBonus);
	case FieldId::STR_BONUS:        return std::format("{}", p.strBonus);
	case FieldId::DEX_BONUS:        return std::format("{}", p.dexBonus);
	case FieldId::CON_BONUS:        return std::format("{}", p.conBonus);
	case FieldId::INT_BONUS:        return std::format("{}", p.intBonus);
	case FieldId::WIS_BONUS:        return std::format("{}", p.wisBonus);
	case FieldId::CHA_BONUS:        return std::format("{}", p.chaBonus);
	case FieldId::IS_SET_MODE:      return p.isSetMode ? "yes" : "no";
	case FieldId::NUTRITION:        return std::format("{}", p.nutritionValue);
	case FieldId::GOLD_AMOUNT:      return std::format("{}", p.goldAmount);
	case FieldId::TILE:             return "(tile)";
	default:                        return "";
	}
}

bool ItemEditor::field_is_string(FieldId f) const
{
	return f == FieldId::NAME || f == FieldId::CATEGORY;
}

bool ItemEditor::field_is_toggle(FieldId f) const
{
	return f == FieldId::ITEM_CLASS
		|| f == FieldId::PICKABLE_TYPE
		|| f == FieldId::CONSUMABLE_EFFECT
		|| f == FieldId::CONSUMABLE_BUFF
		|| f == FieldId::TARGET_MODE
		|| f == FieldId::SCROLL_ANIM
		|| f == FieldId::RANGED
		|| f == FieldId::HAND_REQUIREMENT
		|| f == FieldId::WEAPON_SIZE
		|| f == FieldId::EFFECT
		|| f == FieldId::IS_SET_MODE;
}

void ItemEditor::field_adjust(FieldId f, int delta)
{
	auto clamp_val = [](int v, int d, int lo, int hi) -> int
	{
		return std::clamp(v + d, lo, hi);
	};

	ItemParams& p = m_working;
	switch (f)
	{
	case FieldId::COLOR:
		p.color = clamp_val(p.color, delta, 0, 999);
		break;
	case FieldId::VALUE:
		p.value = clamp_val(p.value, delta, 0, 99999);
		break;
	case FieldId::BASE_WEIGHT:
		p.baseWeight = clamp_val(p.baseWeight, delta, 0, 100);
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
	case FieldId::NUTRITION:
		p.nutritionValue = clamp_val(p.nutritionValue, delta, 0, 9999);
		break;
	case FieldId::GOLD_AMOUNT:
		p.goldAmount = clamp_val(p.goldAmount, delta, 0, 99999);
		break;
	default:
		break;
	}
}

void ItemEditor::field_toggle(FieldId f)
{
	ItemParams& p = m_working;
	constexpr int ITEM_CLASS_COUNT = static_cast<int>(ItemClass::QUEST_ITEM) + 1;
	constexpr int PICKABLE_COUNT = static_cast<int>(PickableType::QUEST_ITEM) + 1;
	constexpr int CONSUMABLE_EFFECT_COUNT = static_cast<int>(ConsumableEffect::FAIL) + 1;
	constexpr int BUFF_COUNT = static_cast<int>(BuffType::HOLD_PERSON) + 1;
	constexpr int TARGET_MODE_COUNT = static_cast<int>(TargetMode::FOV_BUFF) + 1;
	constexpr int SCROLL_ANIM_COUNT = static_cast<int>(ScrollAnimation::EXPLOSION) + 1;
	constexpr int HAND_REQ_COUNT = static_cast<int>(HandRequirement::OFF_HAND_ONLY) + 1;
	constexpr int WEAPON_SIZE_COUNT = static_cast<int>(WeaponSize::GIANT) + 1;
	constexpr int EFFECT_COUNT = static_cast<int>(MagicalEffect::PROTECTION) + 1;

	switch (f)
	{
	case FieldId::ITEM_CLASS:
		p.itemClass = cycle_enum(p.itemClass, ITEM_CLASS_COUNT);
		break;
	case FieldId::PICKABLE_TYPE:
		p.pickableType = cycle_enum(p.pickableType, PICKABLE_COUNT);
		break;
	case FieldId::CONSUMABLE_EFFECT:
		p.consumableEffect = cycle_enum(p.consumableEffect, CONSUMABLE_EFFECT_COUNT);
		break;
	case FieldId::CONSUMABLE_BUFF:
		p.consumableBuffType = cycle_enum(p.consumableBuffType, BUFF_COUNT);
		break;
	case FieldId::TARGET_MODE:
		p.targetMode = cycle_enum(p.targetMode, TARGET_MODE_COUNT);
		break;
	case FieldId::SCROLL_ANIM:
		p.scrollAnimation = cycle_enum(p.scrollAnimation, SCROLL_ANIM_COUNT);
		break;
	case FieldId::RANGED:
		p.ranged = !p.ranged;
		break;
	case FieldId::HAND_REQUIREMENT:
		p.handRequirement = cycle_enum(p.handRequirement, HAND_REQ_COUNT);
		break;
	case FieldId::WEAPON_SIZE:
		p.weaponSize = cycle_enum(p.weaponSize, WEAPON_SIZE_COUNT);
		break;
	case FieldId::EFFECT:
		p.effect = cycle_enum(p.effect, EFFECT_COUNT);
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
	case FieldId::NAME:     m_working_name = std::move(val); break;
	case FieldId::CATEGORY: m_working_category = std::move(val); break;
	default: break;
	}
}

// end of file: ItemEditor.cpp
