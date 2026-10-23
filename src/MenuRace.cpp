// file: MenuRace.cpp
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "GameContext.h"
#include "ListMenu.h"
#include "MenuClass.h"
#include "MenuRace.h"
#include "Player.h"
#include "RandomDice.h"

namespace
{
// Which key picks a race. The key is the menu's own business; a race's name and its
// ability adjustments are the race's, and both are read from it rather than written
// here. Half-Elf takes no key, 'h' being the human's and 'e' the elf's.
struct RaceChoice
{
	PlayerRaceState race;
	char hotkey;
};

constexpr std::array<RaceChoice, 6> RACE_CHOICES{
	RaceChoice{ PlayerRaceState::HUMAN, 'h' },
	RaceChoice{ PlayerRaceState::DWARF, 'd' },
	RaceChoice{ PlayerRaceState::ELF, 'e' },
	RaceChoice{ PlayerRaceState::GNOME, 'g' },
	RaceChoice{ PlayerRaceState::HALFELF, 0 },
	RaceChoice{ PlayerRaceState::HALFLING, 'l' }
};
} // namespace

std::unique_ptr<BaseMenu> make_race_menu(GameContext& ctx)
{
	std::vector<MenuEntry> entries;

	// Choosing a race is one operation whichever entry asks for it. The blueprint holds
	// the race alone; its name and its modifiers are derived from it where they are read.
	auto choose_race = [](PlayerRaceState race, GameContext& chosenCtx)
	{
		chosenCtx.playerBlueprint->playerRaceState = race;
		chosenCtx.menus->push_back(make_class_menu(chosenCtx));
	};

	for (const RaceChoice& choice : RACE_CHOICES)
	{
		const PlayerRaceState race = choice.race;
		auto raceCommand = [choose_race, race](GameContext& commandCtx)
		{
			choose_race(race, commandCtx);
		};
		entries.push_back(
			{
				std::string{ race_display_name(choice.race) },
				choice.hotkey,
				raceCommand,
			});
	}

	// A d6 over the six the menu offers, so a race added to the list is rollable by
	// being on it.
	auto randomCommand = [choose_race](GameContext& randomCtx)
	{
		const size_t rolled = static_cast<size_t>(randomCtx.dice->d6() - 1);
		choose_race(ALL_PLAYER_RACE.at(rolled), randomCtx);
	};
	entries.push_back({ "Random", 'r', randomCommand });

	auto backCommand = [](GameContext& backCtx)
	{
		backCtx.menus->back()->back = true;
	};
	entries.push_back({ "Back", 'b', backCommand });

	return std::make_unique<ListMenu>(
		"SELECT RACE",
		std::move(entries),
		std::function<void(GameContext&)>{},
		std::function<void(GameContext&)>{},
		ctx);
}

// end of file: MenuRace.cpp
