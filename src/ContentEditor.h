// file: ContentEditor.h
#pragma once

#include <string>
#include <vector>

#include "Renderer.h"

class ContentRegistry;
class MonsterRegistry;
class ItemRegistry;

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
private:
	struct Entry
	{
		std::string name;
		std::string itemKey; // for item tab (string key into ItemCreator)
		int entityKey{ 0 }; // for monster tab (MonsterId int cast)
	};

	[[nodiscard]] TileRef current_tile(const MonsterRegistry& monsters) const;
	void assign_tile(TileRef tile, MonsterRegistry& monsters);

	void draw_header(const Renderer& renderer);
	void draw_list(const Renderer& renderer, const MonsterRegistry& monsters, int list_x, int list_y, int list_w, int list_h);
	void draw_browser(const Renderer& renderer, MonsterRegistry& monsters, int panelX, int panelY, int bw, int bh);
	void draw_hint_bar(const Renderer& renderer) const;
	void handle_keyboard();

	ContentRegistry* contentRegistry{ nullptr };
	bool active{ false };
	int tab{ 0 };
	int listScroll{ 0 };
	int listCursor{ 0 };
	int browserSheet{ 0 };
	int browserScroll{ 0 };
	int bufferedChar{ 0 };
	mutable double lastSaveTime{ -10.0 };

	std::vector<Entry> itemEntries;
	std::vector<Entry> monsterEntries;

	[[nodiscard]] std::vector<Entry>& active_entries()
	{
		return tab == 0 ? itemEntries : monsterEntries;
	}

	[[nodiscard]] const std::vector<Entry>& active_entries() const
	{
		return tab == 0 ? itemEntries : monsterEntries;
	}

public:
	void toggle(ContentRegistry& registry, MonsterRegistry& monsters, const ItemRegistry& items);

	void set_char_input(int ch) noexcept { bufferedChar = ch; }

	void update_and_render(const Renderer& renderer, ContentRegistry& registry, MonsterRegistry& monsters);

	[[nodiscard]] bool is_active() const { return active; }
};
