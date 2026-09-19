// file: MonsterFactory.cpp
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "GameContext.h"
#include "LevelManager.h"
#include "MessageSystem.h"
#include "MonsterCreator.h"
#include "MonsterFactory.h"
#include "MonsterRegistry.h"
#include "Monsters.h"
#include "RandomDice.h"
#include "ShopkeeperFactory.h"
#include "Spider.h"
#include "Vector2D.h"

namespace
{
// One monster the table can draw, and the rule for its weight at a dungeon level.
struct MonsterType
{
	std::string name{};
	// Weight at the first level; 0 means never drawn at random (e.g. dungeon_jailer).
	int baseWeight{ 0 };
	int levelMinimum{ 1 };
	// No maximum when 0.
	int levelMaximum{ 0 };
	// How much the weight grows per level past the first; negative shrinks it.
	float levelScaling{ 0.0f };
	std::function<void(Vector2D, GameContext&)> createFunc{};
};

// The spawn table as the registry holds it now: its standard monsters, then its
// custom ones, then the class-based creatures, whose weights are fixed here.
std::vector<MonsterType> spawn_table(const MonsterRegistry& monsters)
{
	std::vector<MonsterType> table{};

	// Registry-driven monsters: adding a new monster only requires a new entry
	// in the registry - nothing here changes.
	for (const auto& [id, params] : monsters.get_standard_monsters())
	{
		table.push_back(
			{
				.name = params.name,
				.baseWeight = params.baseWeight,
				.levelMinimum = params.levelMinimum,
				.levelMaximum = params.levelMaximum,
				.levelScaling = params.levelScaling,
				.createFunc = [id](Vector2D pos, GameContext& ctx)
				{
					ctx.creatures->push_back(MonsterCreator::create(pos, id, ctx));
				},
			});
	}

	// Custom monsters - composed via editor, spawned from saved params
	for (const std::string& key : monsters.get_all_keys())
	{
		if (monsters.is_builtin(key) || monsters.is_class_key(key))
		{
			continue;
		}
		const MonsterParams& params = monsters.get_params(key);
		table.push_back(
			{
				.name = params.name,
				.baseWeight = params.baseWeight,
				.levelMinimum = params.levelMinimum,
				.levelMaximum = params.levelMaximum,
				.levelScaling = params.levelScaling,
				.createFunc = [key](Vector2D pos, GameContext& ctx)
				{
					ctx.creatures->push_back(
						MonsterCreator::create_from_params(pos, ctx.monsterRegistry->get_params(key), ctx));
				},
			});
	}

	// Spiders - class-based (unique web-spinning behaviour)
	table.push_back(
		{
			.name = "Small Spider",
			.baseWeight = 10,
			.levelMinimum = 1,
			.levelMaximum = 0,
			.levelScaling = -0.3f,
			.createFunc = [](Vector2D pos, GameContext& ctx)
			{
				ctx.creatures->push_back(std::make_unique<SmallSpider>(pos, ctx));
			},
		});

	table.push_back(
		{
			.name = "Giant Spider",
			.baseWeight = 10,
			.levelMinimum = 2,
			.levelMaximum = 0,
			.levelScaling = 0.0f,
			.createFunc = [](Vector2D pos, GameContext& ctx)
			{
				ctx.creatures->push_back(std::make_unique<GiantSpider>(pos, ctx));
			},
		});

	table.push_back(
		{
			.name = "Web Spinner",
			.baseWeight = 5,
			.levelMinimum = 3,
			.levelMaximum = 0,
			.levelScaling = 0.2f,
			.createFunc = [](Vector2D pos, GameContext& ctx)
			{
				ctx.creatures->push_back(std::make_unique<WebSpinner>(pos, ctx));
			},
		});

	// Mimic - class-based (disguise logic)
	table.push_back(
		{
			.name = "Mimic",
			.baseWeight = 6,
			.levelMinimum = 2,
			.levelMaximum = 0,
			.levelScaling = 0.5f,
			.createFunc = [](Vector2D pos, GameContext& ctx)
			{
				ctx.creatures->push_back(std::make_unique<Mimic>(pos, ctx));
			},
		});

	// Shopkeeper - class-based (shop inventory logic)
	table.push_back(
		{
			.name = "Shopkeeper",
			.baseWeight = 20,
			.levelMinimum = 1,
			.levelMaximum = 0,
			.levelScaling = 0.0f,
			.createFunc = [](Vector2D pos, GameContext& ctx)
			{
				const int dungeonLevel = ctx.levelManager->get_dungeon_level();
				if (ShopkeeperFactory::should_spawn_shopkeeper(dungeonLevel, ctx))
				{
					ctx.creatures->push_back(ShopkeeperFactory::create_shopkeeper(pos, dungeonLevel, ctx));
					ctx.messageSystem->log("Shopkeeper spawned at level " + std::to_string(dungeonLevel));
				}
				else
				{
					ctx.creatures->push_back(MonsterCreator::create(pos, MonsterId::GOBLIN, ctx));
					ctx.messageSystem->log("Shopkeeper spawn failed, spawned Goblin instead");
				}
			},
		});

	return table;
}

// A monster's weight at a dungeon level: 0 outside its levels or for a base weight of
// 0, otherwise its base weight scaled by level and never below 1.
int calculate_weight(const MonsterType& monster, int dungeonLevel)
{
	auto is_within_levels = [](int level, int minimum, int maximum)
	{
		if (level < minimum)
		{
			return false;
		}
		if (maximum > 0 && level > maximum)
		{
			return false;
		}
		return true;
	};

	// baseWeight == 0 means "never spawn randomly" (e.g. dungeon_jailer)
	if (monster.baseWeight == 0)
	{
		return 0;
	}

	if (!is_within_levels(dungeonLevel, monster.levelMinimum, monster.levelMaximum))
	{
		return 0;
	}

	const float levelFactor = 1.0f + (monster.levelScaling * (dungeonLevel - 1));
	const int weight = static_cast<int>(monster.baseWeight * levelFactor);
	return std::max(1, weight);
}
} // namespace

void MonsterFactory::spawn_random_monster(Vector2D position, int dungeonLevel, GameContext& ctx)
{
	const std::vector<MonsterType> table = spawn_table(*ctx.monsterRegistry);

	int totalWeight = 0;
	std::vector<int> weights;

	for (const MonsterType& monster : table)
	{
		const int weight = calculate_weight(monster, dungeonLevel);
		weights.push_back(weight);
		totalWeight += weight;
	}

	if (totalWeight <= 0)
	{
		ctx.messageSystem->log("No valid monsters for this dungeon level!");
		return;
	}

	const int roll = ctx.dice->roll(1, totalWeight);
	int runningTotal = 0;

	for (size_t index = 0; index < table.size(); ++index)
	{
		runningTotal += weights[index];
		if (roll <= runningTotal)
		{
			table[index].createFunc(position, ctx);
			break;
		}
	}
}

std::vector<MonsterPercentage> MonsterFactory::get_current_distribution(int dungeonLevel, const MonsterRegistry& monsters)
{
	const std::vector<MonsterType> table = spawn_table(monsters);
	std::vector<MonsterPercentage> distribution;

	int totalWeight = 0;
	std::vector<int> weights;

	for (const MonsterType& monster : table)
	{
		const int weight = calculate_weight(monster, dungeonLevel);
		weights.push_back(weight);
		totalWeight += weight;
	}

	for (size_t index = 0; index < table.size(); ++index)
	{
		if (weights[index] > 0)
		{
			const float percentage = static_cast<float>(weights[index]) / totalWeight * 100.0f;
			distribution.push_back({ table[index].name, percentage });
		}
	}

	return distribution;
}
