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
	Player::PlayerRaceState race;
	char hotkey;
};

constexpr std::array<RaceChoice, 6> RACE_CHOICES{
	RaceChoice{ Player::PlayerRaceState::HUMAN, 'h' },
	RaceChoice{ Player::PlayerRaceState::DWARF, 'd' },
	RaceChoice{ Player::PlayerRaceState::ELF, 'e' },
	RaceChoice{ Player::PlayerRaceState::GNOME, 'g' },
	RaceChoice{ Player::PlayerRaceState::HALFELF, 0 },
	RaceChoice{ Player::PlayerRaceState::HALFLING, 'l' }
};
} // namespace

std::unique_ptr<BaseMenu> make_race_menu(GameContext& ctx)
{
	std::vector<MenuEntry> entries;

	// Writing the race is one operation whichever entry asks for it, so the name and
	// the modifiers cannot be set from different races by one of them.
	auto choose_race = [](Player::PlayerRaceState race, GameContext& chosenCtx)
	{
		chosenCtx.playerBlueprint->playerRace = std::string{ race_display_name(race) };
		chosenCtx.playerBlueprint->racialModifier = racial_ability_modifiers(race);
		chosenCtx.menus->push_back(make_class_menu(chosenCtx));
	};

	for (const RaceChoice& choice : RACE_CHOICES)
	{
		auto raceCommand = [choose_race, race = choice.race](GameContext& commandCtx)
		{
			choose_race(race, commandCtx);
		};
		entries.push_back({ std::string{ race_display_name(choice.race) }, choice.hotkey, raceCommand });
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
