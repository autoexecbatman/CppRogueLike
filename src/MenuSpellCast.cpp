#include <algorithm>
#include <cassert>
#include <format>
#include <iterator>
#include <string>

#include "Colors.h"
#include "EquipmentSlot.h"
#include "GameContext.h"
#include "Item.h"
#include "MenuSpellCast.h"
#include "MessageSystem.h"
#include "Pickable.h"
#include "Player.h"
#include "PlayerTurn.h"
#include "Renderer.h"
#include "SpellRegistry.h"
#include "SpellSystem.h"

MenuSpellCast::MenuSpellCast(Player& player, GameContext& ctx)
	: player(player)
{
	assert(ctx.spellRegistry && "MenuSpellCast::MenuSpellCast: called without a spellRegistry");
	assert(ctx.gameState && "MenuSpellCast::MenuSpellCast: called without a gameState");

	populate_spells(ctx.gameState->get_time());

	if (entries.empty())
	{
		// No spells available, so menu() closes on its first frame.
		return;
	}

	assert(ctx.renderer && "MenuSpellCast: renderer required before construction");
	const int tileSize = ctx.renderer->get_tile_size();

	// 45 was a character count handed to menu_new, which reads tiles: 2880 pixels
	// on a 1280-pixel screen. Measured off the lines this menu actually draws.
	int widestText = ctx.renderer->measure_text("Cast Spell (ESC to cancel):");
	for (size_t spellIndex = 0; spellIndex < entries.size(); ++spellIndex)
	{
		widestText = std::max(widestText, ctx.renderer->measure_text(spell_line(spellIndex, *ctx.spellRegistry)));
	}

	// One header row, then a row per spell.
	const int width = panel_tiles_for_text_width(widestText, tileSize);
	const int height = panel_tiles_for_text_rows(
		static_cast<int>(entries.size()) + 1, tileSize, ctx.renderer->get_font_size());
	const int vrows = ctx.renderer->get_viewport_rows();
	const int vcols = ctx.renderer->get_viewport_cols();
	const size_t startX = static_cast<size_t>((vcols - width) / 2);
	const size_t startY = static_cast<size_t>((vrows - height) / 2);
	menu_new(static_cast<size_t>(width), static_cast<size_t>(height), startX, startY, ctx);
}

void MenuSpellCast::populate_spells(int now)
{
	entries.clear();

	// Add memorized spells (string keys)
	for (const auto& key : player.memorizedSpells)
	{
		entries.push_back({ key, "", SpellSystem::ItemPower::CAST_SPELL });
	}

	// Add item-granted spells
	std::ranges::copy(SpellSystem::get_item_granted_spells(player, now), std::back_inserter(entries));
}

std::string MenuSpellCast::spell_line(size_t index, const SpellRegistry& spells) const
{
	const SpellSystem::ItemGrantedSpell& entry = entries[index];
	const char letter = static_cast<char>('a' + static_cast<int>(index));

	// A command word is no spell, so there is nothing in the registry to name it by.
	if (entry.power == SpellSystem::ItemPower::SPEAK_COMMAND_WORD)
	{
		return std::format("{}) Speak the command word [{}]", letter, entry.source);
	}

	const SpellDefinition& definition = spells.get_by_key(entry.key);
	// An item-granted spell names what granted it; a memorised one names its level.
	if (!entry.source.empty())
	{
		return std::format("{}) {} [{}]", letter, definition.name, entry.source);
	}
	return std::format("{}) {} (L{})", letter, definition.name, definition.level);
}

void MenuSpellCast::draw(const SpellRegistry& spells)
{
	menu_draw_box();
	menu_print(1, 0, "Cast Spell (ESC to cancel):");

	for (size_t spellIndex = 0; spellIndex < entries.size(); ++spellIndex)
	{
		const int row = static_cast<int>(spellIndex) + 1;

		if (static_cast<int>(spellIndex) == selectedIndex)
		{
			menu_highlight_on();
		}

		menu_print(1, row, spell_line(spellIndex, spells));

		if (static_cast<int>(spellIndex) == selectedIndex)
		{
			menu_highlight_off();
		}
	}
}

void MenuSpellCast::handle_selection(GameContext& ctx)
{
	if (selectedIndex < 0 || selectedIndex >= static_cast<int>(entries.size()))
	{
		return;
	}

	const SpellSystem::ItemGrantedSpell& entry = entries[static_cast<size_t>(selectedIndex)];

	// Speaking the word is the round's action, as a casting is.
	if (entry.power == SpellSystem::ItemPower::SPEAK_COMMAND_WORD)
	{
		Item* helm = player.get_equipped_item(EquipmentSlot::HEAD);
		assert(helm && "MenuSpellCast::handle_selection: a command word was offered with no helm worn");
		speak_command_word(*helm, player, ctx);
		spend_player_action(ctx, TIME_UNITS_PER_ROUND);
		menu_set_run_false();
		return;
	}

	const std::string key = entry.key;
	const bool isMemorized = entry.source.empty();
	const bool spendsTheHelm = entry.power == SpellSystem::ItemPower::CAST_SPELL_ONCE_A_DAY;

	// onCastComplete fires when casting is over, whether or not the spell worked: Wisdom can fizzle it.
	// &playerRef binds to the Player member, not to a local that would dangle after handle_selection().
	auto onCastComplete = [key, isMemorized, spendsTheHelm, &playerRef = player](GameContext& innerCtx)
	{
		if (isMemorized)
		{
			std::erase(playerRef.memorizedSpells, key);
		}
		// Recorded before the round is spent, so the day runs from the moment of use.
		if (spendsTheHelm)
		{
			Item* helm = playerRef.get_equipped_item(EquipmentSlot::HEAD);
			assert(helm && "MenuSpellCast: the helm's daily spell was cast with no helm worn");
			assert(innerCtx.gameState && "MenuSpellCast: the helm's daily spell was cast without a gameState");
			record_helm_use(*helm, innerCtx.gameState->get_time());
		}
		spend_player_spellcasting(innerCtx);
	};

	const SpellSource source = isMemorized ? SpellSource::MEMORIZED : SpellSource::ITEM;
	SpellSystem::cast_spell_by_key(key, player, source, std::move(onCastComplete), ctx);
	menu_set_run_false();
}

void MenuSpellCast::on_key(GameContext& ctx)
{
	if (lastKey == GameKey::ESCAPE)
	{
		menu_set_run_false();
	}
	else if (lastKey == GameKey::UP)
	{
		if (selectedIndex > 0)
		{
			selectedIndex--;
		}
	}
	else if (lastKey == GameKey::DOWN)
	{
		if (selectedIndex < static_cast<int>(entries.size()) - 1)
		{
			selectedIndex++;
		}
	}
	else if (lastKey == GameKey::ENTER || lastKey == GameKey::SPACE)
	{
		handle_selection(ctx);
	}
	else if (lastChar >= 'a' && lastChar <= 'z')
	{
		int selection = lastChar - 'a';
		if (selection < static_cast<int>(entries.size()))
		{
			selectedIndex = selection;
			handle_selection(ctx);
		}
	}
}

void MenuSpellCast::menu(GameContext& ctx)
{
	assert(ctx.spellRegistry && "MenuSpellCast::menu: called without a spellRegistry");

	if (entries.empty())
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "No spells available.", MessageCompletion::FINISHED);
		menu_set_run_false();
		return;
	}

	menu_key_listen();
	draw(*ctx.spellRegistry);
	on_key(ctx);
}
