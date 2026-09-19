#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "InventoryOperations.h"
#include "Ai.h"
#include "AiMonsterConfused.h"
#include "Colors.h"
#include "Map.h"
#include "Web.h"
#include "DamageInfo.h"
#include "GameContext.h"
#include "AnimationSystem.h"
#include "Persistent.h"
#include "BuffSystem.h"
#include "BuffType.h"
#include "FloatingTextSystem.h"
#include "MessageSystem.h"
#include "ShopKeeper.h"
#include "TileConfig.h"
#include "Actor.h"
#include "Attacker.h"
#include "MonsterAttacker.h"
#include "EquipmentSlot.h"
#include "InventoryData.h"
#include "Item.h"
#include "Creature.h"
#include "Player.h"
#include "Pickable.h"

//==Creature==
void Creature::load(const json& j)
{
	Actor::load(j); // Call base class load
	baseStrength = j["strength"];
	exceptionalStrength = j.at("exceptionalStrength").get<int>();
	baseDexterity = j["dexterity"];
	baseConstitution = j["constitution"];
	baseIntelligence = j["intelligence"];
	baseWisdom = j["wisdom"];
	baseCharisma = j["charisma"];
	creatureLevel = j["playerLevel"];
	gold = j["gold"];
	gender = j["gender"];
	naturalAttack = j.value("naturalAttack", std::string{});
	ethics = static_cast<Ethics>(j.at("ethics").get<int>());
	morality = static_cast<Morality>(j.at("morality").get<int>());
	undead = j.at("undead").get<bool>();
	awarenessTurns = j.at("awarenessTurns").get<int>();
	webStuckTurns = j.at("webStuckTurns").get<int>();
	webStrength = j.at("webStrength").get<int>();
	creatureClass = static_cast<CreatureClass>(j.value("creatureClass", static_cast<int>(CreatureClass::MONSTER)));
	hitDie = j.value("hitDie", 8);
	attacksPerRound = j.value("attacksPerRound", 1.0f);
	damageResistance = j.value("dr", 0);
	thaco = j.value("thaco", 20);
	if (j.contains("attacker"))
	{
		attacker = std::make_unique<MonsterAttacker>(*this, DamageInfo{});
		attacker->load(j["attacker"]);
	}
	// Load health pool data
	if (j.contains("healthPool"))
	{
		const auto& healthJson = j["healthPool"];
		if (healthPool && healthJson.contains("hpMax"))
		{
			healthPool->set_max_hp(healthJson.at("hpMax").get<int>());
			healthPool->set_hp(healthJson.at("hp").get<int>());
			healthPool->set_hp_base(healthJson.at("hpBase").get<int>());
			healthPool->set_temp_hp(healthJson.at("tempHp").get<int>());
		}
	}
	// Load constitution tracker state
	if (j.contains("constitutionTracker"))
	{
		const auto& constJson = j["constitutionTracker"];
		if (constJson.contains("lastConstitution"))
		{
			constitutionTracker->set_last_constitution(constJson.at("lastConstitution").get<int>());
		}
	}
	// Load experience reward
	if (j.contains("experienceReward"))
	{
		experienceReward = std::make_unique<ExperienceReward>(0);
		experienceReward->load(j["experienceReward"]);
	}
	// Load armor class
	if (j.contains("armorClass"))
	{
		const auto& acJson = j["armorClass"];
		armorClass = std::make_unique<ArmorClass>(10);
		armorClass->set_armor_class(acJson.at("armorClass").get<int>());
		armorClass->set_base_armor_class(acJson.at("baseArmorClass").get<int>());
	}
	if (j.contains("ai"))
	{
		ai = Ai::create(j["ai"]);
	}
	if (j.contains("inventoryData"))
	{
		inventoryData = CreatureInventory(50); // Default capacity
		InventoryOperations::load_inventory(inventoryData, j["inventoryData"]);
	}
	if (j.contains("shop"))
	{
		shop = ShopKeeper::create(j["shop"]);
	}

	// Load unified buff system
	if (j.contains("activeBuffs"))
	{
		activeBuffs.clear();
		for (const auto& buffJson : j["activeBuffs"])
		{
			Buff buff{};
			buff.type = static_cast<BuffType>(buffJson.at("type").get<int>());
			buff.value = buffJson.at("value").get<int>();
			buff.turnsRemaining = buffJson.at("turnsRemaining").get<int>();
			buff.isSetEffect = buffJson.at("isSetEffect").get<bool>();
			for (const auto& saveJson : buffJson.at("opponentSaves"))
			{
				buff.opponentSaves.push_back(OpponentSave{
					saveJson.at("opponent").get<UniqueId::IdType>(),
					saveJson.at("isMade").get<bool>() });
			}
			activeBuffs.push_back(buff);
		}
	}
}

void Creature::save(json& j)
{
	Actor::save(j); // Call base class save
	j["strength"] = baseStrength;
	j["exceptionalStrength"] = exceptionalStrength;
	j["dexterity"] = baseDexterity;
	j["constitution"] = baseConstitution;
	j["intelligence"] = baseIntelligence;
	j["wisdom"] = baseWisdom;
	j["charisma"] = baseCharisma;
	j["playerLevel"] = creatureLevel;
	j["gold"] = gold;
	j["gender"] = gender;
	j["naturalAttack"] = naturalAttack;
	j["ethics"] = static_cast<int>(ethics);
	j["morality"] = static_cast<int>(morality);
	j["undead"] = undead;
	j["awarenessTurns"] = awarenessTurns;
	j["webStuckTurns"] = webStuckTurns;
	j["webStrength"] = webStrength;
	j["creatureClass"] = static_cast<int>(creatureClass);
	j["hitDie"] = hitDie;
	j["attacksPerRound"] = attacksPerRound;
	j["dr"] = damageResistance;
	j["thaco"] = thaco;
	if (attacker)
	{
		json attackerJson;
		attacker->save(attackerJson);
		// Only write the key if the strategy produced content.
		// PlayerAttacker::save() is a no-op — no damageInfo to persist.
		if (!attackerJson.empty())
		{
			j["attacker"] = attackerJson;
		}
	}
	// Save health pool data
	if (healthPool)
	{
		json healthJson;
		healthJson["hpMax"] = healthPool->get_max_hp();
		healthJson["hp"] = healthPool->get_hp();
		healthJson["hpBase"] = healthPool->get_hp_base();
		healthJson["tempHp"] = healthPool->get_temp_hp();
		j["healthPool"] = healthJson;
	}
	// Save constitution tracker state
	json constJson;
	if (const auto lastConstitution = get_last_constitution(); lastConstitution.has_value())
	{
		constJson["lastConstitution"] = *lastConstitution;
	}
	j["constitutionTracker"] = constJson;
	// Save experience reward
	if (experienceReward)
	{
		json expJson;
		experienceReward->save(expJson);
		j["experienceReward"] = expJson;
	}
	// Save armor class
	if (armorClass)
	{
		json acJson;
		acJson["armorClass"] = armorClass->get_armor_class();
		acJson["baseArmorClass"] = armorClass->get_base_armor_class();
		j["armorClass"] = acJson;
	}
	if (ai)
	{
		json aiJson;
		ai->save(aiJson);
		j["ai"] = aiJson;
	}
	// Always save inventory data since it always exists
	json inventoryJson;
	InventoryOperations::save_inventory(inventoryData, inventoryJson);
	j["inventoryData"] = inventoryJson;
	if (shop)
	{
		json shopJson;
		shop->save(shopJson);
		j["shop"] = shopJson;
	}

	// Save unified buff system
	json buffsJson = json::array();
	for (const auto& buff : activeBuffs)
	{
		json buffJson;
		buffJson["type"] = static_cast<int>(buff.type);
		buffJson["value"] = buff.value;
		buffJson["turnsRemaining"] = buff.turnsRemaining;
		buffJson["isSetEffect"] = buff.isSetEffect;
		json savesJson = json::array();
		for (const OpponentSave& save : buff.opponentSaves)
		{
			savesJson.push_back(json{ { "opponent", save.opponent }, { "isMade", save.isMade } });
		}
		buffJson["opponentSaves"] = savesJson;
		buffsJson.push_back(buffJson);
	}
	j["activeBuffs"] = buffsJson;
}

void Creature::update_creature_state(GameContext& ctx)
{
	if (!invisibleTile.is_valid() && ctx.tileConfig)
	{
		invisibleTile = ctx.tileConfig->get("TILE_INVISIBLE");
	}

	ctx.buffSystem->restore_loaded_buff_states(*this);
	ctx.buffSystem->update_creature_buffs(*this);

	update_armor_class(ctx);
	update_constitution_bonus(ctx);
}

// Recomputes armour class, and writes the arithmetic to the log for the player.
//
// Only the player's breakdown is logged: it is there so a human can check the
// numbers against the rules, and nobody reads a goblin's. ArmorClass itself
// does not know that - it reports, and this decides who hears.
void Creature::update_armor_class(GameContext& ctx)
{
	const ArmorClassBreakdown breakdown = armorClass->update(*this, ctx);

	if (!is_player() || !breakdown.changed)
	{
		return;
	}

	ctx.messageSystem->log(std::format(
		"Armor Class updated: {} -> {} (Base: {}, Dex: {:+}, Equipment: {:+}, Temp: {:+})",
		breakdown.previous,
		breakdown.total,
		breakdown.base,
		breakdown.dexterity,
		breakdown.equipment,
		breakdown.temporary));

	// Name each contributing item, skipping the slots that gave nothing.
	const ArmorClassSource sources[] = { breakdown.armor, breakdown.shield, breakdown.ring, breakdown.helm };
	for (const ArmorClassSource& source : sources)
	{
		if (!source.name.empty())
		{
			ctx.messageSystem->log(std::format("  {:+} from {}", source.bonus, source.name));
		}
	}
}

void Creature::set_hit_dice(int hitPoints)
{
	assert(hitPoints > 0 && "Creature::set_hit_dice called with a roll at or below zero");
	healthPool = std::make_unique<HealthPool>(hitPoints);

	// Recorded now, so the first tick finds nothing to apply.
	constitutionTracker->set_last_constitution(get_constitution());
}

void Creature::update_constitution_bonus(GameContext& ctx)
{
	const std::optional<int> oldCon = get_last_constitution();
	const auto result = constitutionTracker->apply_constitution_changes(*this, ctx);

	if (result.hpDifference == 0)
	{
		return;
	}

	set_max_hp(get_max_hp() + result.hpDifference);
	set_hp(get_hp() + result.hpDifference);

	// A first application accounts for the score the creature was made with;
	// only a move between two scores is a change worth telling the player.
	if (is_player() && !result.firstApplication)
	{
		if (result.hpDifference > 0)
		{
			ctx.messageSystem->message(GREEN_BLACK_PAIR,
				std::format("Constitution increased from {} to {}! You gain {} hit points.", *oldCon, get_constitution(), result.hpDifference),
				true);
		}
		else
		{
			ctx.messageSystem->message(RED_BLACK_PAIR,
				std::format("Constitution decreased from {} to {}! You lose {} hit points.", *oldCon, get_constitution(), -result.hpDifference),
				true);
		}
	}

	if (get_hp() <= 0)
	{
		set_hp(0);
		if (is_player())
		{
			ctx.messageSystem->message(RED_BLACK_PAIR, "Your life force has been drained beyond recovery. You die!", true);
		}
		else
		{
			ctx.messageSystem->log(std::format("{} dies from stat drain.", get_name()));
		}
		die(ctx);
	}
}

// Applies damage and shows the number floating off the creature.
//
// HealthPool does the arithmetic and FloatingTextSystem decides how the number
// looks; this states who was hurt and joins the two.
int Creature::take_damage(int damage, GameContext& ctx, DamageType damageType)
{
	assert(damageType != DamageType::FIRE && damageType != DamageType::COLD && "Creature::take_damage: fire and cold arrive as ResistedDamage from DamageResolver::reduce_dice");
	return take_damage_hit_points(damage, ctx, damageType);
}

int Creature::take_damage(const DamageResolver::ResistedDamage& damage, GameContext& ctx)
{
	return take_damage_hit_points(damage.hit_points(), ctx, damage.damage_type());
}

void Creature::take_damage_and_check_death(const DamageResolver::ResistedDamage& damage, GameContext& ctx)
{
	take_damage(damage, ctx);
	if (is_dead())
	{
		die(ctx);
	}
}

int Creature::take_damage_hit_points(int damage, GameContext& ctx, DamageType damageType)
{
	const int actualDamage = healthPool->take_damage(*this, damage, ctx, damageType);

	if (ctx.floatingText)
	{
		ctx.floatingText->spawn_damage(
			position,
			actualDamage,
			is_player() ? DamageSubject::PLAYER : DamageSubject::MONSTER);
	}

	return actualDamage;
}

void Creature::take_damage_and_check_death(int damage, GameContext& ctx, DamageType damageType)
{
	take_damage(damage, ctx, damageType);
	if (is_dead())
	{
		die(ctx);
	}
}

// the actor update -- monsters only; Player overrides this
void Creature::update(GameContext& ctx)
{
	update_creature_state(ctx);

	// A held creature spends its turn struggling rather than acting.
	if (is_webbed() && try_break_web(ctx) == WebEscape::STILL_STUCK)
	{
		return;
	}

	assert(ai && "Creature::update called with null ai");
	ai->update(*this, ctx);
}

void Creature::update_awareness(const GameContext& ctx)
{
	// An invisible player is not seen even in plain line of sight.
	if (ctx.map->is_in_fov(position) && !ctx.player()->is_invisible())
	{
		awarenessTurns = AWARENESS_TURNS;
		return;
	}

	if (awarenessTurns > 0)
	{
		--awarenessTurns;
	}
}

void Creature::set_body_plan(std::vector<EquipmentSlot> slots) noexcept
{
	bodyPlan = std::move(slots);
}

bool Creature::has_slot(EquipmentSlot slot) const noexcept
{
	return std::ranges::find(bodyPlan, slot) != bodyPlan.end();
}

// Puts an item into a slot this creature's body provides.
//
// Example:
//   orc.wear(ItemCreator::create("long_sword", pos, ctx), EquipmentSlot::RIGHT_HAND);
//   orc.get_attack_name(); // -> "long sword"
void Creature::wear(std::unique_ptr<Item> item, EquipmentSlot slot)
{
	assert(item && "Creature::wear called with no item");
	assert(has_slot(slot) && "Creature::wear called with a slot this body does not have");

	item->add_state(ActorState::IS_EQUIPPED);
	equippedItems.push_back(EquippedItem(std::move(item), slot));
}

// What this creature strikes with. The held weapon wins, because a monster
// that picks up a sword is fighting with the sword.
//
// Example:
//   troll.get_attack_name();  // -> "Claws", from its body
//   orc.get_attack_name();    // -> "long sword", from its main hand
std::string Creature::get_attack_name() const
{
	if (const Item* mainHand = get_equipped_item(EquipmentSlot::RIGHT_HAND))
	{
		return mainHand->get_name();
	}

	if (!naturalAttack.empty())
	{
		return naturalAttack;
	}

	return "unarmed";
}

// What sits in one slot, or nothing when the slot is empty or the creature's
// body plan does not grant it.
//
// Example:
//   orc.get_equipped_item(EquipmentSlot::RIGHT_HAND);  // -> the long sword
//   wolf.get_equipped_item(EquipmentSlot::RIGHT_HAND); // -> nullptr
int Creature::worn_resistance_strength(DamageType damageType) const noexcept
{
	int greatest = 0;
	for (const EquippedItem& equipped : equippedItems)
	{
		const Item& item = *equipped.item;
		// An item's effect and its enhancement are two claims; the larger stands.
		greatest = std::max(greatest, item.get_enhancement().resistance_strength(damageType));
		if (item.behavior)
		{
			greatest = std::max(greatest, get_item_resistance_strength(*item.behavior, damageType));
		}
	}
	return greatest;
}

bool Creature::can_equip(const Item& item, EquipmentSlot slot) const noexcept
{
	// Basic slot validation
	if (slot == EquipmentSlot::NONE)
	{
		return false;
	}

	// Check if item has behavior component (weapons/armor)
	if (!item.behavior)
	{
		return false;
	}

	// Slot-specific validation
	switch (slot)
	{

	case EquipmentSlot::RIGHT_HAND:
	case EquipmentSlot::LEFT_HAND:
	{
		// Hand slots can hold weapons or shields - use proper item type system
		if (!item.is_weapon() && !item.is_shield())
		{
			return false; // Not a weapon or shield
		}

		// Shields can only go in left hand
		if (item.is_shield() && slot != EquipmentSlot::LEFT_HAND)
		{
			return false;
		}

		// Two-handed weapons can only go in right hand
		if (item.is_two_handed_weapon() && slot != EquipmentSlot::RIGHT_HAND)
		{
			return false;
		}

		// Check if trying to equip something in left hand when two-handed weapon is equipped
		if (slot == EquipmentSlot::LEFT_HAND)
		{
			auto* rightHandItem = get_equipped_item(EquipmentSlot::RIGHT_HAND);
			if (rightHandItem && rightHandItem->is_two_handed_weapon())
			{
				return false; // Can't equip anything in left hand when two-handed weapon equipped
			}
		}

		break;
	}

	case EquipmentSlot::BODY:
	{
		// Body slot can only hold armor - use proper item type system
		if (!item.is_armor())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::MISSILE_WEAPON:
	{
		// Missile weapon slot can only hold ranged weapons - use ItemClass system
		if (!item.is_ranged_weapon())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::HEAD:
	{
		if (!item.is_helmet())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::NECK:
	{
		if (!item.is_amulet())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::RIGHT_RING:
	case EquipmentSlot::LEFT_RING:
	{
		if (!item.is_ring())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::GAUNTLETS:
	{
		if (!item.is_gauntlets())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::GIRDLE:
	{
		if (!item.is_girdle())
		{
			return false;
		}
		break;
	}

	case EquipmentSlot::TOOL:
	{
		if (!item.is_tool())
		{
			return false;
		}
		break;
	}

	default:
	{
		// Other slots (CLOAK, BRACERS, BOOTS, MISSILES) - no items defined yet
		break;
	}

	}

	return true;
}


bool Creature::has_ranged_weapon() const noexcept
{
	const Item* missile = get_equipped_item(EquipmentSlot::MISSILE_WEAPON);
	return missile != nullptr && missile->is_ranged_weapon();
}

Item* Creature::get_equipped_item(EquipmentSlot slot) const noexcept
{
	auto worn = std::ranges::find_if(equippedItems, matches_slot(slot));

	return (worn != equippedItems.end()) ? worn->item.get() : nullptr;
}

// Binds this creature into a web. The caller decides what is announced.
//
// Example:
//   creature.apply_web_effect(4, 2, web); // held for 4 turns by a strength-2 web
//   creature.is_webbed();                 // -> true
void Creature::apply_web_effect(int duration, int strength, Web* web)
{
	webStuckTurns = duration;
	webStrength = strength;
	trappingWeb = web;
}

// Rolls 1d100 against a chance built from this creature's strength against the
// web's, then ages the binding by one turn. Both escapes destroy the web.
//
// Example:
//   creature.try_break_web(ctx); // -> WebEscape::STILL_STUCK, one turn spent
//   creature.try_break_web(ctx); // -> WebEscape::BROKE_FREE on a good roll
WebEscape Creature::try_break_web(GameContext& ctx)
{
	// Stronger creatures tear loose more often; stronger webs hold better.
	const int breakChance = std::max(10, 20 + (get_strength() * 5) - (webStrength * 10));

	if (ctx.dice->d100() <= breakChance)
	{
		release_from_web();
		return WebEscape::BROKE_FREE;
	}

	webStuckTurns--;

	// The binding ran out; the creature is free whatever it rolled.
	if (webStuckTurns <= 0)
	{
		release_from_web();
		return WebEscape::STRUGGLED_FREE;
	}

	return WebEscape::STILL_STUCK;
}

// Clears the binding and destroys whatever web held it.
void Creature::release_from_web()
{
	if (trappingWeb)
	{
		trappingWeb->destroy();
		trappingWeb = nullptr;
	}

	webStuckTurns = 0;
	webStrength = 0;
}

void Creature::apply_confusion(int nbTurns)
{
	ai = std::make_unique<AiMonsterConfused>(nbTurns, std::move(ai));
}

void Creature::unequip(Item& item, GameContext& ctx)
{
	// Check if the item is actually equipped
	if (item.has_state(ActorState::IS_EQUIPPED))
	{
		// Remove the equipped state
		item.remove_state(ActorState::IS_EQUIPPED);

		// Unequipping a weapon leaves the creature striking with its body
		if (item.is_weapon())
		{
			ctx.messageSystem->log("Unequipped weapon - now unarmed");
		}
	}
}

void Creature::drop(Item& item, GameContext& ctx)
{
	[[maybe_unused]] auto is_null = [](const auto& invItem) { return !invItem; };
	assert(std::ranges::none_of(inventoryData.items, is_null));

	auto matches_item = [&item](const auto& invItem) { return invItem.get() == &item; };
	auto matches = inventoryData.items | std::views::filter(matches_item);

	if (std::ranges::empty(matches))
	{
		return;
	}

	auto& foundPtr = matches.front();
	foundPtr->position = position;

	if (foundPtr->has_state(ActorState::IS_EQUIPPED))
	{
		unequip(*foundPtr, ctx);
	}

	auto addResult = InventoryOperations::add_item(*ctx.floorInventory, std::move(foundPtr));
	if (addResult.has_value())
	{
		InventoryOperations::optimize_inventory_storage(inventoryData);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You dropped the item.", true);
	}
}

TileRef Creature::get_display_tile() const noexcept
{
	if (is_invisible())
	{
		return invisibleTile; // Instant access, no string lookup
	}
	return Actor::get_display_tile();
}

int Creature::get_display_color() const noexcept
{
	if (is_invisible())
	{
		return CYAN_BLACK_PAIR;
	}
	return Actor::get_display_color();
}

void Creature::die(GameContext& ctx)
{
	// Monster death: message, reward, animation, drop items, create corpse
	ctx.messageSystem->append_message_part(actorData.color, std::format("{}", actorData.name));
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " is dead.\n");
	ctx.messageSystem->finalize_message();

	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You get ");
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, std::format("{}", get_xp()));
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " experience points.\n");
	ctx.messageSystem->finalize_message();

	assert(ctx.player() != nullptr && "Creature::die requires a live player in context");
	ctx.player_concrete().on_kill_reward(get_xp(), ctx);

	if (ctx.animSystem)
	{
		ctx.animSystem->spawn_death(position);
	}

	assert(std::ranges::none_of(inventoryData.items, [](const auto& i) { return !i; }));
	for (auto& item : inventoryData.items)
	{
		item->position = position;
		[[maybe_unused]] const auto dropOnDeathResult = InventoryOperations::add_item(*ctx.floorInventory, std::move(item));
		assert(dropOnDeathResult.has_value());
	}
	inventoryData.items.clear();

	// Create a corpse on the floor in place of the creature.
	auto corpse = std::make_unique<Item>(position, actorData);
	corpse->actorData.name = std::format("dead {}", get_name());
	corpse->actorData.tile = ctx.tileConfig->get("TILE_CORPSE");
	corpse->enhancement.weight = get_corpse_weight();
	corpse->behavior = CorpseFood{ 0 };
	[[maybe_unused]] const auto placeCorpseResult = InventoryOperations::add_item(*ctx.floorInventory, std::move(corpse));
	assert(placeCorpseResult.has_value());
}

namespace
{
// What one worn item does to one ability: a value it sets the score to, an amount it
// adds, or neither. Ability-raising gauntlets, girdles and amulets set or add as their
// data says; a worn item's Strength or Dexterity enhancement adds.
//
// Example, for Strength:
//   worn_ability_effect(girdleOfHillGiantStrength, BuffType::STRENGTH);   // -> { 19, 0 }
//   worn_ability_effect(swimmingGauntlets, BuffType::STRENGTH);           // -> { 0, 2 }
//   worn_ability_effect(longSword, BuffType::STRENGTH);                   // -> { 0, 0 }
struct WornAbilityEffect
{
	int setTo{ 0 };
	int add{ 0 };
};

WornAbilityEffect worn_ability_effect(const Item& item, BuffType ability) noexcept
{
	WornAbilityEffect effect{};

	// An enhancement's bonus adds; only Strength and Dexterity have ever applied.
	if (ability == BuffType::STRENGTH)
	{
		effect.add += item.get_enhancement().strengthBonus;
	}
	if (ability == BuffType::DEXTERITY)
	{
		effect.add += item.get_enhancement().dexterityBonus;
	}

	if (!item.behavior)
	{
		return effect;
	}

	auto from_stat_boost = [ability, &effect](const auto& boost)
	{
		using T = std::decay_t<decltype(boost)>;
		if constexpr (std::is_same_v<T, JewelryAmulet> || std::is_same_v<T, Gauntlets> || std::is_same_v<T, Girdle>)
		{
			int amount = 0;
			switch (ability)
			{
			case BuffType::STRENGTH:
			{
				amount = boost.strBonus;
				break;
			}
			case BuffType::DEXTERITY:
			{
				amount = boost.dexBonus;
				break;
			}
			case BuffType::CONSTITUTION:
			{
				amount = boost.conBonus;
				break;
			}
			case BuffType::INTELLIGENCE:
			{
				amount = boost.intBonus;
				break;
			}
			case BuffType::WISDOM:
			{
				amount = boost.wisBonus;
				break;
			}
			case BuffType::CHARISMA:
			{
				amount = boost.chaBonus;
				break;
			}
			default:
			{
				break;
			}
			}

			if (boost.isSetMode)
			{
				effect.setTo = amount;
			}
			else
			{
				effect.add += amount;
			}
		}
	};
	std::visit(from_stat_boost, *item.behavior);
	return effect;
}
} // namespace

int Creature::get_exceptional_strength() const noexcept
{
	int exceptional = exceptionalStrength;

	// A worn item that sets Strength to 18 brings its own percentile; the higher counts.
	auto from_setting_item = [&exceptional](const auto& boost)
	{
		using T = std::decay_t<decltype(boost)>;
		if constexpr (std::is_same_v<T, JewelryAmulet> || std::is_same_v<T, Gauntlets> || std::is_same_v<T, Girdle>)
		{
			if (boost.isSetMode && boost.strBonus == 18)
			{
				exceptional = std::max(exceptional, boost.exceptionalStrength);
			}
		}
	};
	for (const EquippedItem& worn : equippedItems)
	{
		assert(worn.item && "an equipment slot holds a null item");
		if (worn.item->behavior)
		{
			std::visit(from_setting_item, *worn.item->behavior);
		}
	}
	return exceptional;
}

// AD&D 2e: an ability score is the creature's own, or the highest value any buff or worn
// item sets it to if that is higher, plus every buff's and worn item's addition. Nothing
// is written back, so what is removed takes exactly its own contribution with it.
int Creature::calculate_effective_stat(int base_value, BuffType type) const noexcept
{
	auto matchesType = [type](const Buff& b)
	{
		return b.type == type;
	};
	auto matchingBuffs = activeBuffs | std::views::filter(matchesType);

	int highestSet = 0; // Highest SET effect (Potion of Giant Strength → 18)
	int sumOfAdds = 0; // Sum of ADD effects (Strength spell +1, Gauntlets +2)

	for (const auto& buff : matchingBuffs)
	{
		if (buff.isSetEffect)
		{
			highestSet = std::max(highestSet, buff.value);
		}
		else
		{
			sumOfAdds += buff.value;
		}
	}

	// Worn items count exactly as buffs do.
	for (const EquippedItem& worn : equippedItems)
	{
		assert(worn.item && "an equipment slot holds a null item");
		const WornAbilityEffect effect = worn_ability_effect(*worn.item, type);
		highestSet = std::max(highestSet, effect.setTo);
		sumOfAdds += effect.add;
	}

	// AD&D 2e: SET effects replace base (if higher), ADD effects always stack
	// Example: base=14, SET=18, ADD=+2 → MAX(14,18) + 2 = 20
	int effectiveBase = (highestSet > 0) ? std::max(base_value, highestSet) : base_value;
	return effectiveBase + sumOfAdds;
}