// file: ContentEditor.h
#pragma once

#include <string>
#include <vector>

#include "../Renderer/Renderer.h"

class ContentRegistry;

// ContentEditor -- developer tool for assigning sprite tiles to items and monsters.
//
// F3           toggle editor
// Tab          switch Items / Monsters tab
// Up/Down      navigate entity list
// Left/Right   cycle sprite sheets
// Click list   select entity
// Click tile   assign tile to selected entity
// Ctrl+S       save to data/content/tiles.json

class ContentEditor
{
public:
	void toggle(ContentRegistry& registry);
	void set_char_input(int ch) noexcept { m_buffered_char = ch; }
	void update_and_render(const Renderer& renderer, ContentRegistry& registry);

	[[nodiscard]] bool is_active() const { return m_active; }

private:
	struct Entry
	{
		std::string name;
		std::string item_key; // for item tab (string key into ItemCreator)
		int entity_key{ 0 }; // for monster tab (MonsterId int cast)
	};

	[[nodiscard]] TileRef current_tile() const;
	void assign_tile(TileRef tile);

	void draw_header(const Renderer& renderer);
	void draw_list(const Renderer& renderer, int list_x, int list_y, int list_w, int list_h);
	void draw_browser(const Renderer& renderer, int ox, int oy, int bw, int bh);
	void draw_hint_bar(const Renderer& renderer) const;
	void handle_keyboard();

	ContentRegistry* m_registry{ nullptr };
	bool m_active{ false };
	int m_tab{ 0 };
	int m_list_scroll{ 0 };
	int m_list_cursor{ 0 };
	int m_browser_sheet{ 0 };
	int m_browser_scroll{ 0 };
	int m_buffered_char{ 0 };
	mutable double m_last_save_time{ -10.0 };

	std::vector<Entry> m_item_entries;
	std::vector<Entry> m_monster_entries;

	[[nodiscard]] std::vector<Entry>& active_entries()
	{
		return m_tab == 0 ? m_item_entries : m_monster_entries;
	}

	[[nodiscard]] const std::vector<Entry>& active_entries() const
	{
		return m_tab == 0 ? m_item_entries : m_monster_entries;
	}
};
