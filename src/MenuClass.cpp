// file: MenuClass.cpp
#include <memory>
#include <string>
#include <vector>

#include "CreatureClass.h"
#include "GameContext.h"
#include "ListMenu.h"
#include "MenuAbilityScores.h"
#include "MenuClass.h"
#include "Player.h"
#include "RandomDice.h"

std::unique_ptr<BaseMenu> make_class_menu(GameContext& ctx)
{
	std::vector<MenuEntry> entries;

	auto fighterCommand = [](GameContext& ctx)
	{
		ctx.playerBlueprint->creatureClass = CreatureClass::FIGHTER;
		ctx.menus->push_back(std::make_unique<MenuAbilityScores>(ctx));
	};
	entries.push_back({ std::string{ class_display_name(CreatureClass::FIGHTER) }, 'f', fighterCommand });

	auto rogueCommand = [](GameContext& ctx)
	{
		ctx.playerBlueprint->creatureClass = CreatureClass::ROGUE;
		ctx.menus->push_back(std::make_unique<MenuAbilityScores>(ctx));
	};
	entries.push_back({ std::string{ class_display_name(CreatureClass::ROGUE) }, 'r', rogueCommand });

	auto clericCommand = [](GameContext& ctx)
	{
		ctx.playerBlueprint->creatureClass = CreatureClass::CLERIC;
		ctx.menus->push_back(std::make_unique<MenuAbilityScores>(ctx));
	};
	entries.push_back({ std::string{ class_display_name(CreatureClass::CLERIC) }, 'c', clericCommand });

	auto wizardCommand = [](GameContext& ctx)
	{
		ctx.playerBlueprint->creatureClass = CreatureClass::WIZARD;
		ctx.menus->push_back(std::make_unique<MenuAbilityScores>(ctx));
	};
	entries.push_back({ std::string{ class_display_name(CreatureClass::WIZARD) }, 'z', wizardCommand });

	auto randomCommand = [](GameContext& ctx)
	{
		switch (ctx.dice->d4())
		{

		case 1:
		{
			ctx.playerBlueprint->creatureClass = CreatureClass::FIGHTER;
			break;
		}

		case 2:
		{
			ctx.playerBlueprint->creatureClass = CreatureClass::ROGUE;
			break;
		}

		case 3:
		{
			ctx.playerBlueprint->creatureClass = CreatureClass::WIZARD;
			break;
		}

		case 4:
		{
			ctx.playerBlueprint->creatureClass = CreatureClass::CLERIC;
			break;
		}

		default:
		{
			break;
		}
		}
		ctx.menus->push_back(std::make_unique<MenuAbilityScores>(ctx));
	};
	entries.push_back({ "Random", 'x', randomCommand });

	auto backCommand = [](GameContext& ctx)
	{
		ctx.menus->back()->back = true;
	};
	entries.push_back({ "Back", 'b', backCommand });

	return std::make_unique<ListMenu>(
		"SELECT CLASS",
		std::move(entries),
		std::function<void(GameContext&)>{},
		std::function<void(GameContext&)>{},
		ctx);
}

// end of file: MenuClass.cpp
