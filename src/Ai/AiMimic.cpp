// file: AiMimic.cpp
#include <algorithm>
#include <format>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Actor/Actor.h"
#include "../Actor/Creature.h"
#include "../Actor/InventoryOperations.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Combat/DamageInfo.h"
#include "../Core/GameContext.h"
#include "../Factories/ItemCreator.h"
#include "../Factories/MonsterCreator.h"
#include "../Items/ItemClassification.h"
#include "../Persistent/Persistent.h"
#include "../Systems/ContentRegistry.h"
#include "../Systems/MessageSystem.h"
#include "AiMimic.h"
#include "AiMonster.h"

// Configuration constants (NOT serialized — same for all mimics)
constexpr int DISGUISE_CHANGE_RATE = 200;
constexpr int CONSUMPTION_COOLDOWN_TURNS = 3;
constexpr int CONSUMPTION_RADIUS = 1;
constexpr int MAX_CONFUSION_DURATION = 5;
constexpr int MAX_GOLD_DR_BONUS = 2;
constexpr int MAX_WEAPON_DAMAGE = 6;
constexpr int MAX_ARMOR_DR_BONUS = 3;
constexpr int ITEMS_FOR_TRANSFORMATION = 5;
constexpr int HEALTH_BONUS = 1;
constexpr int DR_BONUS = 1;
constexpr int CONFUSION_BONUS = 1;

// OCP-compliant: data-driven item bonus mapping
enum class MimicBonusType
{
	HEALTH,
	CONFUSION,
	ATTACK,
	DEFENSE_GOLD,
	DEFENSE_ARMOR,
	NONE
};

static const std::unordered_map<ItemClass, MimicBonusType> item_bonus_map = {
	{ ItemClass::POTION, MimicBonusType::HEALTH },
	{ ItemClass::FOOD, MimicBonusType::HEALTH },
	{ ItemClass::SCROLL, MimicBonusType::CONFUSION },
	{ ItemClass::GOLD_COIN, MimicBonusType::DEFENSE_GOLD },
	{ ItemClass::ARMOR, MimicBonusType::DEFENSE_ARMOR },
	{ ItemClass::DAGGER, MimicBonusType::ATTACK },
	{ ItemClass::SWORD, MimicBonusType::ATTACK },
	{ ItemClass::GREAT_SWORD, MimicBonusType::ATTACK },
	{ ItemClass::AXE, MimicBonusType::ATTACK },
	{ ItemClass::HAMMER, MimicBonusType::ATTACK },
	{ ItemClass::MACE, MimicBonusType::ATTACK },
	{ ItemClass::STAFF, MimicBonusType::ATTACK },
	{ ItemClass::BOW, MimicBonusType::ATTACK },
	{ ItemClass::CROSSBOW, MimicBonusType::ATTACK },
};

// Single source of truth: which item appearances a mimic can adopt.
// Called at fresh construction (Mimic ctor) and as lazy init after load.
std::vector<Disguise> build_mimic_disguises(ContentRegistry& registry)
{
	auto fromItem = [&registry](std::string_view key) -> Disguise
	{
		const auto& p = ItemCreator::get_params(key);
		return { registry.get_tile(key), std::string(p.name), p.color };
	};

	return {
		fromItem("gold_coin"),
		fromItem("health_potion"),
		fromItem("scroll_lightning"),
		fromItem("short_sword"),
		fromItem("food_ration"),
	};
}

AiMimic::AiMimic(std::vector<Disguise> initialDisguises)
	: possibleDisguises(std::move(initialDisguises))
{
}

void AiMimic::update(Creature& owner, GameContext& ctx)
{
	// Lazy init after load: possibleDisguises is empty when created via Ai::create().
	if (possibleDisguises.empty())
	{
		possibleDisguises = build_mimic_disguises(*ctx.contentRegistry);
	}

	if (owner.destructible->is_dead())
	{
		return;
	}

	if (isDisguised)
	{
		disguiseChangeCounter++;

		if (disguiseChangeCounter >= DISGUISE_CHANGE_RATE)
		{
			change_disguise(owner, ctx);
			disguiseChangeCounter = 0;
			ctx.messageSystem->log("Mimic changed disguise");
		}

		check_revealing(owner, ctx);
	}
	else
	{
		const bool itemConsumed = consume_nearby_items(owner, ctx);

		if (!itemConsumed)
		{
			AiMonster::update(owner, ctx);
		}
	}
}

[[nodiscard]] bool AiMimic::consume_nearby_items(Creature& owner, GameContext& ctx)
{
	if (isDisguised || owner.destructible->is_dead())
	{
		return false;
	}

	++consumptionCooldown;
	if (consumptionCooldown < CONSUMPTION_COOLDOWN_TURNS)
	{
		return false;
	}
	consumptionCooldown = 0;

	if (ctx.inventoryData->items.empty())
	{
		return false;
	}

	ctx.messageSystem->log(std::format("Checking for items. Inventory size: {}", ctx.inventoryData->items.size()));

	std::vector<size_t> itemsToRemove;
	bool itemConsumed = false;

	for (size_t i = 0; i < ctx.inventoryData->items.size(); ++i)
	{
		const auto& item = ctx.inventoryData->items[i];
		if (!item)
		{
			continue;
		}

		const int itemDistance = owner.get_tile_distance(item->position);
		ctx.messageSystem->log(std::format("Item at distance {}: {}", itemDistance, item->actorData.name));

		if (itemDistance <= CONSUMPTION_RADIUS)
		{
			if (!item->behavior)
			{
				ctx.messageSystem->log(std::format("Mimic found non-pickable item, skipping: {}", item->actorData.name));
				continue;
			}

			ctx.messageSystem->append_message_part(RED_YELLOW_PAIR, "The mimic ");
			ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "consumes the ");
			ctx.messageSystem->append_message_part(item->actorData.color, item->actorData.name);
			ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "!");
			ctx.messageSystem->finalize_message();

			ctx.messageSystem->log(std::format("Mimic consuming item: {}", item->actorData.name));

			++itemsConsumed;
			apply_item_bonus(owner, item->itemClass, ctx);

			if (itemsConsumed >= ITEMS_FOR_TRANSFORMATION)
			{
				transform_to_greater_mimic(owner, ctx);
			}

			itemsToRemove.push_back(i);
			itemConsumed = true;
			break;
		}
	}

	for (size_t index : std::views::reverse(itemsToRemove))
	{
		ctx.messageSystem->log(std::format("Removing consumed item at index {}", index));

		if (index < ctx.inventoryData->items.size() && ctx.inventoryData->items[index])
		{
			InventoryOperations::remove_item_at(*ctx.inventoryData, index);
		}
	}

	return itemConsumed;
}

void AiMimic::apply_item_bonus(Creature& owner, ItemClass itemClass, GameContext& ctx)
{
	if (!item_bonus_map.contains(itemClass))
	{
		return;
	}

	const MimicBonusType bonusType = item_bonus_map.at(itemClass);

	switch (bonusType)
	{

	case MimicBonusType::HEALTH:
	{
		boost_health(owner, HEALTH_BONUS, ctx);
		ctx.messageSystem->log(std::format("Mimic gained {} HP", HEALTH_BONUS));
		break;
	}

	case MimicBonusType::CONFUSION:
	{
		boost_confusion_power(ctx);
		break;
	}

	case MimicBonusType::DEFENSE_GOLD:
	{
		boost_defense(owner, DR_BONUS, MAX_GOLD_DR_BONUS, ctx);
		if (owner.destructible->get_dr() <= MAX_GOLD_DR_BONUS)
		{
			ctx.messageSystem->log(std::format("Mimic gained {} DR from gold", DR_BONUS));
		}
		break;
	}

	case MimicBonusType::DEFENSE_ARMOR:
	{
		boost_defense(owner, DR_BONUS, MAX_ARMOR_DR_BONUS, ctx);
		if (owner.destructible->get_dr() <= MAX_ARMOR_DR_BONUS)
		{
			ctx.messageSystem->log(std::format("Mimic gained {} DR from armor", DR_BONUS));
		}
		break;
	}

	case MimicBonusType::ATTACK:
	{
		boost_attack(owner, ctx);
		break;
	}

	case MimicBonusType::NONE:
	{
		break;
	}
	}
}

void AiMimic::boost_health(Creature& owner, int amount, GameContext& ctx)
{
	owner.destructible->set_max_hp(owner.destructible->get_max_hp() + amount);
	const int newCurrentHp = std::min(
		owner.destructible->get_hp() + amount,
		owner.destructible->get_max_hp());
	owner.destructible->set_hp(newCurrentHp);
	ctx.messageSystem->log(std::format("Mimic gained {} health from food", amount));
}

void AiMimic::boost_defense(Creature& owner, int amount, int maxDR, GameContext& ctx)
{
	const int currentDR = owner.destructible->get_dr();
	if (currentDR < maxDR)
	{
		owner.destructible->set_dr(currentDR + amount);
	}
}

void AiMimic::boost_attack(Creature& owner, GameContext& ctx)
{
	const auto& currentDamage = owner.attacker->get_damage_info();
	if (currentDamage.maxDamage < MAX_WEAPON_DAMAGE)
	{
		const int newMaxDamage = std::min(currentDamage.maxDamage + 1, MAX_WEAPON_DAMAGE);
		const DamageInfo improvedDamage(
			currentDamage.minDamage,
			newMaxDamage,
			std::format("1d{}", newMaxDamage));
		owner.attacker->set_damage_info(improvedDamage);
		ctx.messageSystem->log(std::format("Mimic improved attack to {}", improvedDamage.displayRoll));
	}
}

void AiMimic::boost_confusion_power(GameContext& ctx)
{
	confusionDuration = std::min(confusionDuration + CONFUSION_BONUS, MAX_CONFUSION_DURATION);
	ctx.messageSystem->log(std::format("Mimic increased confusion duration to {}", confusionDuration));
}

void AiMimic::transform_to_greater_mimic(Creature& owner, GameContext& ctx)
{
	owner.actorData.tile = MonsterCreator::get_tile(MonsterId::MIMIC);
	owner.actorData.color = RED_YELLOW_PAIR;
	owner.actorData.name = "greater mimic";
	ctx.messageSystem->log("Mimic transformed into greater mimic");
}

void AiMimic::check_revealing(Creature& owner, GameContext& ctx)
{
	int distanceToPlayer = owner.get_tile_distance(ctx.player->position);
	ctx.messageSystem->log("Mimic distance to player: " + std::to_string(distanceToPlayer));

	if (distanceToPlayer <= revealDistance)
	{
		isDisguised = false;
		owner.actorData.tile = MonsterCreator::get_tile(MonsterId::MIMIC);
		owner.actorData.name = "mimic";
		owner.actorData.color = RED_YELLOW_PAIR;
		owner.add_state(ActorState::BLOCKS);

		ctx.messageSystem->log("Mimic revealed itself!");

		if (ctx.dice->d20() > ctx.player->get_wisdom())
		{
			ctx.messageSystem->append_message_part(WHITE_GREEN_PAIR, "The ");
			ctx.messageSystem->append_message_part(RED_YELLOW_PAIR, "mimic");
			ctx.messageSystem->append_message_part(WHITE_GREEN_PAIR, " reveals itself and confuses you!");
			ctx.messageSystem->finalize_message();

			ctx.player->add_state(ActorState::IS_CONFUSED);
			ctx.player->ai->apply_confusion(confusionDuration);
			ctx.messageSystem->log("Applied confusion to player for " + std::to_string(confusionDuration) + " turns");
		}
		else
		{
			ctx.messageSystem->append_message_part(RED_YELLOW_PAIR, "A mimic");
			ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " reveals itself but you resist its confusion!");
			ctx.messageSystem->finalize_message();
			ctx.messageSystem->log("Player resisted mimic confusion");
		}
	}
}

void AiMimic::change_disguise(Creature& owner, GameContext& ctx)
{
	if (!isDisguised)
	{
		return;
	}

	if (!possibleDisguises.empty())
	{
		const size_t index = ctx.dice->roll(0, static_cast<int>(possibleDisguises.size()) - 1);
		const auto& chosen = possibleDisguises.at(index);
		owner.actorData.tile = chosen.tile;
		owner.actorData.name = chosen.name;
		owner.actorData.color = chosen.color;
	}
	else
	{
		throw "possibleDisguises is empty in change_disguise()";
	}

	owner.remove_state(ActorState::BLOCKS);
}

void AiMimic::load(const json& j)
{
	AiMonster::load(j);

	if (j.contains("disguiseChangeCounter"))
	{
		disguiseChangeCounter = j.at("disguiseChangeCounter").get<int>();
	}

	if (j.contains("consumptionCooldown"))
	{
		consumptionCooldown = j.at("consumptionCooldown").get<int>();
	}

	if (j.contains("isDisguised"))
	{
		isDisguised = j.at("isDisguised").get<bool>();
	}

	if (j.contains("revealDistance"))
	{
		revealDistance = j.at("revealDistance").get<int>();
	}

	if (j.contains("confusionDuration"))
	{
		confusionDuration = j.at("confusionDuration").get<int>();
	}

	if (j.contains("itemsConsumed"))
	{
		itemsConsumed = j.at("itemsConsumed").get<int>();
	}
}

void AiMimic::save(json& j)
{
	AiMonster::save(j);

	// Override the MONSTER type written by AiMonster::save — we are MIMIC.
	j["type"] = static_cast<int>(AiType::MIMIC);

	j["disguiseChangeCounter"] = disguiseChangeCounter;
	j["consumptionCooldown"] = consumptionCooldown;
	j["isDisguised"] = isDisguised;
	j["revealDistance"] = revealDistance;
	j["confusionDuration"] = confusionDuration;
	j["itemsConsumed"] = itemsConsumed;
}
