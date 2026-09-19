#include <algorithm>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "ContentRegistry.h"
#include "GameContext.h"
#include "Item.h"
#include "ItemCreator.h"
#include "ItemEnhancements.h"
#include "ItemRegistry.h"
#include "Pickable.h"
#include "RandomDice.h"
#include "Renderer.h"
#include "Vector2D.h"

namespace
{

// ---------------------------------------------------------------------------
// Behavior construction
// ---------------------------------------------------------------------------
template <typename T>
T create_stat_behavior(const ItemParams& params)
{
	T item;
	item.strBonus = params.strBonus;
	item.dexBonus = params.dexBonus;
	item.conBonus = params.conBonus;
	item.intBonus = params.intBonus;
	item.wisBonus = params.wisBonus;
	item.chaBonus = params.chaBonus;
	item.effect = params.effect;
	item.bonus = params.effectBonus;
	item.isSetMode = params.isSetMode;
	item.exceptionalStrength = params.exceptionalStrength;
	return item;
}

ItemBehavior create_behavior(const ItemParams& params)
{
	switch (params.pickableType)
	{

	case PickableType::CONSUMABLE:
	{
		return Consumable{
			params.consumableEffect,
			params.consumableAmount,
			params.duration,
			params.consumableBuffType,
			params.isSetMode
		};
	}

	case PickableType::TARGETED_SCROLL:
	{
		return TargetedScroll{
			params.targetMode,
			params.scrollAnimation,
			params.range,
			params.damage,
			params.confuseTurns,
			params.consumableBuffType,
			params.duration
		};
	}

	case PickableType::TELEPORTER:
	{
		return Teleporter{};
	}

	case PickableType::IDENTIFY_SCROLL:
	{
		return IdentifyScroll{};
	}

	case PickableType::WEAPON:
	{
		return Weapon{ params.ranged, params.handRequirement, params.weaponSize };
	}

	case PickableType::SHIELD:
	{
		return Shield{};
	}

	case PickableType::ARMOR:
	{
		return Armor{ params.acBonus };
	}

	case PickableType::QUEST_ITEM:
	{
		return Amulet{};
	}

	case PickableType::MAGICAL_HELM:
	{
		return MagicalHelm{ params.effect, params.effectBonus };
	}

	case PickableType::MAGICAL_RING:
	{
		return MagicalRing{ params.effect, params.effectBonus };
	}

	case PickableType::JEWELRY_AMULET:
	{
		return create_stat_behavior<JewelryAmulet>(params);
	}

	case PickableType::GAUNTLETS:
	{
		return create_stat_behavior<Gauntlets>(params);
	}

	case PickableType::GIRDLE:
	{
		return create_stat_behavior<Girdle>(params);
	}

	case PickableType::FOOD:
	{
		return Food{ params.nutritionValue };
	}

	case PickableType::GOLD_COIN:
	{
		// A gold item is worth what it is authored to be worth. A rolled pile is the
		// exception and sets its own amount afterwards, in create_with_gold_amount.
		return Gold{ params.value };
	}

	case PickableType::DUNGEON_KEY:
	{
		return DungeonKey{};
	}

	default:
	{
		throw std::runtime_error(
			std::format("ItemCreator: unhandled pickable_type {}",
				static_cast<int>(params.pickableType)));
	}
	}
}

// An item built from params: its behaviour, key, class, value, carry weight and the
// tile the content registry holds for its key.
std::unique_ptr<Item> make_item(std::string_view key, const ItemParams& params, Vector2D pos, ContentRegistry& tiles)
{
	TileRef tile = tiles.get_tile(key);
	auto item = std::make_unique<Item>(
		pos,
		ActorData{ tile, std::string{ params.name }, params.color });
	item->behavior = create_behavior(params);
	item->itemKey = std::string{ key };
	item->itemClass = params.itemClass;
	item->set_value(params.value);
	// What it costs to carry, which is a different question from how often it appears.
	item->enhancement.weight = params.weight;
	return item;
}

} // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::unique_ptr<Item> ItemCreator::create(std::string_view key, Vector2D pos, GameContext& ctx)
{
	return make_item(key, ctx.itemRegistry->get_params(key), pos, *ctx.contentRegistry);
}

std::unique_ptr<Item> ItemCreator::create_with_gold_amount(Vector2D pos, int goldAmount, GameContext& ctx)
{
	const ItemParams& params = ctx.itemRegistry->get_params("gold_coin");
	TileRef tile = ctx.contentRegistry->get_tile("gold_coin");
	auto item = std::make_unique<Item>(
		pos,
		ActorData{ tile, std::string{ params.name }, params.color });
	item->behavior = Gold{ goldAmount };
	item->itemKey = "gold_coin";
	item->itemClass = params.itemClass;
	item->set_value(goldAmount);
	return item;
}

std::unique_ptr<Item> ItemCreator::create_with_enhancement(
	std::string_view key,
	Vector2D pos,
	PrefixType prefix,
	SuffixType suffix,
	GameContext& ctx)
{
	auto item = create(key, pos, ctx);
	item->apply_enhancement(ItemEnhancement(prefix, suffix));

	return item;
}

std::unique_ptr<Item> ItemCreator::create_gold_pile(Vector2D pos, GameContext& ctx)
{
	const int goldAmount = ctx.dice->roll(5, 20);

	return create_with_gold_amount(pos, goldAmount, ctx);
}

std::unique_ptr<Item> ItemCreator::create_random_of_category(
	std::string_view category,
	Vector2D pos,
	GameContext& ctx,
	int dungeonLevel)
{
	struct Candidate
	{
		std::string key;
		int weight;
	};

	std::vector<Candidate> candidates;
	int totalWeight = 0;

	const ItemRegistry& items = *ctx.itemRegistry;
	for (const std::string& key : items.get_all_keys())
	{
		const ItemParams& params = items.get_params(key);
		if (params.category != category || params.baseWeight <= 0)
		{
			continue;
		}
		if (dungeonLevel < params.levelMin)
		{
			continue;
		}
		if (params.levelMax > 0 && dungeonLevel > params.levelMax)
		{
			continue;
		}

		const float levelFactor = 1.0f + (params.levelScaling * static_cast<float>(dungeonLevel - 1));
		const int weight = std::max(1, static_cast<int>(params.baseWeight * levelFactor));
		candidates.push_back({ key, weight });
		totalWeight += weight;
	}

	if (candidates.empty())
	{
		return nullptr;
	}

	int roll = ctx.dice->roll(1, totalWeight);
	for (const auto& [key, weight] : candidates)
	{
		roll -= weight;
		if (roll <= 0)
		{
			return create(key, pos, ctx);
		}
	}

	return create(candidates.back().key, pos, ctx);
}
