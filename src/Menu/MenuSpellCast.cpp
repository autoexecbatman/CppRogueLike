#include "MenuSpellCast.h"
#include "../Core/GameContext.h"
#include "../ActorTypes/Player.h"
#include "../Items/Jewelry.h"
#include "../Items/MagicalItemEffects.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/RenderingManager.h"
#include "../Colors/Colors.h"

// TODO: Rule of 5.
MenuSpellCast::MenuSpellCast(GameContext& ctx, Player& player)
	: player(player), ctx(ctx)
{
}

void MenuSpellCast::populate_spells()
{
	availableSpells.clear();
	spellSources.clear();

	// Add memorized spells
	for (const auto& spell : player.memorizedSpells)
	{
		availableSpells.push_back(spell);
		spellSources.push_back("");
	}

	// Add item-granted spells
	const auto itemSpells = SpellSystem::get_item_granted_spells(player);
	for (const auto& itemSpell : itemSpells)
	{
		availableSpells.push_back(itemSpell.spell);
		spellSources.push_back(itemSpell.source);
	}
}

void MenuSpellCast::draw_content()
{
	menu_clear();
	box(menuWindow, 0, 0);
	menu_print(2, 1, "Cast Spell (ESC to cancel):");

	for (size_t i = 0; i < availableSpells.size(); ++i)
	{
		const auto& def = SpellSystem::get_spell_definition(availableSpells[i]);

		if (static_cast<int>(i) == selectedIndex)
		{
			menu_highlight_on();
		}

		if (!spellSources[i].empty())
		{
			menu_print(2, static_cast<int>(i) + 2,
				std::string(1, 'a' + static_cast<char>(i)) + ") " + def.name + " [" + spellSources[i] + "]");
		}
		else
		{
			menu_print(2, static_cast<int>(i) + 2,
				std::string(1, 'a' + static_cast<char>(i)) + ") " + def.name + " (L" + std::to_string(def.level) + ")");
		}

		if (static_cast<int>(i) == selectedIndex)
		{
			menu_highlight_off();
		}
	}

	menu_refresh();
}

void MenuSpellCast::handle_selection()
{
	if (selectedIndex >= 0 && selectedIndex < static_cast<int>(availableSpells.size()))
	{
		SpellId spell = availableSpells[selectedIndex];
		if (SpellSystem::cast_spell(spell, player, ctx))
		{
			// Only consume memorized spells (not item-granted)
			if (spellSources[selectedIndex].empty())
			{
				// Find and remove from memorized list
				auto it = std::find(player.memorizedSpells.begin(), player.memorizedSpells.end(), spell);
				if (it != player.memorizedSpells.end())
				{
					player.memorizedSpells.erase(it);
				}
			}
			*ctx.game_status = GameStatus::NEW_TURN;
			menu_set_run_false();
		}
	}
}

void MenuSpellCast::menu(GameContext& ctx)
{
	populate_spells();

	if (availableSpells.empty())
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "No spells available.", true);
		return;
	}

	const int height = static_cast<int>(availableSpells.size()) + 4;
	const int width = 45;
	const size_t startY = (LINES - height) / 2;
	const size_t startX = (COLS - width) / 2;

	menu_new(height, width, startY, startX, ctx);

	while (run)
	{
		draw_content();
		menu_key_listen();

		switch (keyPress)
		{
		case 27: // ESC
			menu_set_run_false();
			break;

		case KEY_UP:
			if (selectedIndex > 0)
			{
				selectedIndex--;
			}
			break;

		case KEY_DOWN:
			if (selectedIndex < static_cast<int>(availableSpells.size()) - 1)
			{
				selectedIndex++;
			}
			break;

		case '\n': // Enter
		case ' ':  // Space
			handle_selection();
			break;

		default:
			// Letter selection (a-z)
			if (keyPress >= 'a' && keyPress <= 'z')
			{
				int selection = keyPress - 'a';
				if (selection >= 0 && selection < static_cast<int>(availableSpells.size()))
				{
					selectedIndex = selection;
					handle_selection();
				}
			}
			break;
		}
	}

	clear();
	ctx.rendering_manager->render(ctx);
	refresh();

	menu_delete();
}
