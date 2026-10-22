#include <algorithm>
#include <cassert>
#include <concepts>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "AbilityAllocation.h"
#include "Actor.h"
#include "Ai.h"
#include "AiMonsterConfused.h"
#include "AnimationSystem.h"
#include "Attacker.h"
#include "BuffSystem.h"
#include "BuffType.h"
#include "Colors.h"
#include "ConstitutionAttributes.h"
#include "Creature.h"
#include "DamageInfo.h"
#include "DataManager.h"
#include "Drowning.h"
#include "EquipmentSlot.h"
#include "FloatingTextSystem.h"
#include "GameBalance.h"
#include "GameContext.h"
#include "InventoryData.h"
#include "InventoryOperations.h"
#include "Item.h"
#include "Lifespan.h"
#include "Map.h"
#include "MessageSystem.h"
#include "MonsterAttacker.h"
#include "Persistent.h"
#include "Pickable.h"
#include "Player.h"
#include "PlayerRaceState.h"
#include "ShopKeeper.h"
#include "TileConfig.h"
#include "TurnSchedule.h"
#include "WeaponDamageRegistry.h"
#include "Web.h"

// An entry exists because something put an item in a slot, so one holding nothing is
// a slot that lost what it was carrying. Asserted here, at the single site every
// entry passes through - wear, equip_item, the put-back in unequip_item and the load
// path all reach it - rather than at each of the readers that dereference it.
EquippedItem::EquippedItem(std::unique_ptr<Item> worn, EquipmentSlot wornIn)
	: item(std::move(worn)), slot(wornIn)
{
	assert(item && "an equipment slot is built around the item in it");
}

// Equipment comparison predicates for DRY compliance
namespace
{
constexpr auto matches_unique_id = [](uint64_t uniqueId)
{
	return [uniqueId](const EquippedItem& equipped)
	{
		return equipped.item->uniqueId == uniqueId;
	};
};
} // namespace

namespace
{
// A behaviour that names a magical effect. Consumable carries an `effect` of its own
// kind, so the type is part of the test rather than the member name alone.
template <typename Behavior>
concept GrantsMagicalEffect = requires(const Behavior& behavior) {
	{ behavior.effect } -> std::same_as<const MagicalEffect&>;
};
} // namespace

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
		using BoostType = std::decay_t<decltype(boost)>;
		if constexpr (std::is_same_v<BoostType, JewelryAmulet> || std::is_same_v<BoostType, Gauntlets> || std::is_same_v<BoostType, Girdle>)
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

			if (boost.application == BuffApplication::SET)
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

// Whether the item is a girdle of giant strength, which is the girdle that sets Strength.
//
// Example:
//   is_giant_strength_girdle(girdleOfHillGiantStrength); // -> true
//   is_giant_strength_girdle(gauntletsOfOgrePower); // -> false, gauntlets
bool is_giant_strength_girdle(const Item& item) noexcept
{
	const Girdle* girdle = item.behavior ? std::get_if<Girdle>(&*item.behavior) : nullptr;
	return girdle != nullptr && girdle->application == BuffApplication::SET && girdle->strBonus > 0;
}

// The item's gauntlets if they set Strength, as gauntlets of ogre power do, or null.
//
// Example:
//   strength_setting_gauntlets(gauntletsOfOgrePower); // -> their Gauntlets
//   strength_setting_gauntlets(gauntletsOfSwimmingAndClimbing); // -> nullptr, they add
const Gauntlets* strength_setting_gauntlets(const Item& item) noexcept
{
	const Gauntlets* gauntlets = item.behavior ? std::get_if<Gauntlets>(&*item.behavior) : nullptr;
	return gauntlets != nullptr && gauntlets->application == BuffApplication::SET && gauntlets->strBonus > 0 ? gauntlets : nullptr;
}
} // namespace

//==Creature==
void Creature::load(const json& source)
{
	Actor::load(source); // Call base class load
	baseStrength = source.at("strength").get<int>();
	exceptionalStrength = source.at("exceptionalStrength").get<int>();
	baseDexterity = source.at("dexterity").get<int>();
	baseConstitution = source.at("constitution").get<int>();
	baseIntelligence = source.at("intelligence").get<int>();
	baseWisdom = source.at("wisdom").get<int>();
	baseCharisma = source.at("charisma").get<int>();
	creatureLevel = source.at("playerLevel").get<int>();
	gold = source.at("gold").get<int>();
	gender = source.at("gender").get<std::string>();
	naturalAttack = source.at("naturalAttack").get<std::string>();
	ethics = parse_ethics(source.at("ethics").get<std::string>());
	morality = parse_morality(source.at("morality").get<std::string>());
	undead = source.at("undead").get<bool>();
	awareUntilTime = source.at("awareUntilTime").get<int>();
	attitude = parse_attitude(source.at("attitude").get<std::string>());
	displaceable = source.at("displaceable").get<bool>();
	morale = source.at("morale").get<int>();
	corpseWeight = source.at("corpseWeight").get<int>();
	corpseName = source.at("corpseName").get<std::string>();
	corpseNutrition = source.at("corpseNutrition").get<int>();
	corpseTaste = source.at("corpseTaste").get<std::string>();
	corpseEdibility = corpse_edibility_from_record(source.at("corpseEdible").get<bool>());
	bodyPlan.clear();
	for (const auto& slotName : source.at("bodyPlan"))
	{
		bodyPlan.push_back(parse_equipment_slot(slotName.get<std::string>()));
	}

	// Absent when the creature is not holding a breath, which is the usual case, so the
	// key is written only when there is one and read only when it is there.
	airUntilTime.reset();
	if (source.contains("airUntilTime"))
	{
		airUntilTime = source.at("airUntilTime").get<int>();
	}
	webStuckTurns = source.at("webStuckTurns").get<int>();
	webStrength = source.at("webStrength").get<int>();
	// trappingWeb stays empty: webs are spell tiles, which the save does not hold (issues/0167).
	creatureClass = parse_creature_class(source.at("creatureClass").get<std::string>());
	hitDie = source.at("hitDie").get<int>();
	attacksPerRound = source.at("attacksPerRound").get<float>();
	actionDelay = source.at("actionDelay").get<int>();
	// Null for a monster, which has no age; every field of a character's is required.
	const json& lifespanRecord = source.at("lifespan");
	lifespan.reset();
	if (!lifespanRecord.is_null())
	{
		lifespan = Lifespan{
			static_cast<PlayerRaceState>(lifespanRecord.at("race").get<int>()),
			lifespanRecord.at("startingAge").get<int>(),
			lifespanRecord.at("maximumAge").get<int>(),
			lifespanRecord.at("yearsLived").get<int>(),
			lifespanRecord.at("magicalYears").get<int>()
		};
	}
	nextActionTime = source.at("nextActionTime").get<int>();
	damageResistance = source.at("dr").get<int>();
	thaco = source.at("thaco").get<int>();
	// Written only while a dose is working, so a record without one carries none.
	pendingPoison.reset();
	if (source.contains("pendingPoison"))
	{
		const auto& poisonJson = source.at("pendingPoison");
		PendingPoison dose{};
		dose.roundsUntilOnset = poisonJson.at("roundsUntilOnset").get<int>();
		dose.damage = poisonJson.at("damage").get<int>();
		dose.effect = parse_buff_type(poisonJson.at("effect").get<std::string>());
		dose.effectValue = poisonJson.at("effectValue").get<int>();
		dose.effectRounds = poisonJson.at("effectRounds").get<int>();
		pendingPoison = dose;
	}
	if (source.contains("attacker"))
	{
		attacker = std::make_unique<MonsterAttacker>(*this, DamageInfo{});
		attacker->load(source["attacker"]);
	}
	// A pool that was saved is built from its record, as the other components are: the game
	// loads a level's monsters into fresh objects, which have none to fill in.
	if (source.contains("healthPool"))
	{
		const auto& healthJson = source.at("healthPool");
		healthPool = std::make_unique<HealthPool>(healthJson.at("hpMax").get<int>());
		healthPool->set_hp(healthJson.at("hp").get<int>());
		healthPool->set_hp_base(healthJson.at("hpBase").get<int>());
		healthPool->set_temp_hp(healthJson.at("tempHp").get<int>());
		healthPool->set_unregenerable_damage(healthJson.at("unregenerableDamage").get<int>());
	}
	// Load constitution tracker state; the score inside is saved only once one is recorded.
	{
		const auto& constJson = source.at("constitutionTracker");
		if (constJson.contains("lastConstitution"))
		{
			constitutionTracker->set_last_constitution(constJson.at("lastConstitution").get<int>());
		}
	}
	// Load experience reward
	if (source.contains("experienceReward"))
	{
		experienceReward = std::make_unique<ExperienceReward>(0);
		experienceReward->load(source["experienceReward"]);
	}
	// Load armor class
	if (source.contains("armorClass"))
	{
		const auto& acJson = source["armorClass"];
		armorClass = std::make_unique<ArmorClass>(10);
		armorClass->set_armor_class(acJson.at("armorClass").get<int>());
		armorClass->set_base_armor_class(acJson.at("baseArmorClass").get<int>());
	}
	if (source.contains("ai"))
	{
		ai = Ai::create(source["ai"]);
	}
	inventoryData = CreatureInventory(50); // Default capacity
	InventoryOperations::load_inventory(inventoryData, source.at("inventoryData"));
	if (source.contains("shop"))
	{
		shop = ShopKeeper::create(source["shop"]);
	}

	// Load unified buff system
	{
		activeBuffs.clear();
		for (const auto& buffJson : source.at("activeBuffs"))
		{
			Buff buff{};
			buff.type = parse_buff_type(buffJson.at("type").get<std::string>());
			buff.value = buffJson.at("value").get<int>();
			const json& expiry = buffJson.at("expiryTime");
			buff.expiryTime = expiry.is_null() ? std::nullopt : std::optional<int>{ expiry.get<int>() };
			buff.application = parse_buff_application(buffJson.at("application").get<std::string>());
			for (const auto& saveJson : buffJson.at("opponentSaves"))
			{
				buff.opponentSaves.push_back(OpponentSave{
					saveJson.at("opponent").get<UniqueId::IdType>(),
					saveJson.at("isMade").get<bool>() });
			}
			activeBuffs.push_back(buff);
		}

		// The buff-imposed states were not saved; they are derived here, before anything reads them.
		BuffSystem::sync_states(*this);
	}

	equippedItems.clear();
	for (const auto& itemEntry : source.at("equippedItems"))
	{
		const EquipmentSlot slot = parse_equipment_slot(itemEntry.at("slot").get<std::string>());
		auto item = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", ColorPairId::WHITE_BLACK });
		item->load(itemEntry.at("item"));
		equippedItems.emplace_back(std::move(item), slot);
	}
}

void Creature::save(json& output)
{
	Actor::save(output); // Call base class save
	// A buff-imposed state is derived from the buffs saved below, so the save holds only the buffs.
	json ownStates = json::array();
	for (const json& state : output.at("states"))
	{
		if (!BuffSystem::is_buff_imposed(state.get<ActorState>()))
		{
			ownStates.push_back(state);
		}
	}
	output["states"] = ownStates;
	output["strength"] = baseStrength;
	output["exceptionalStrength"] = exceptionalStrength;
	output["dexterity"] = baseDexterity;
	output["constitution"] = baseConstitution;
	output["intelligence"] = baseIntelligence;
	output["wisdom"] = baseWisdom;
	output["charisma"] = baseCharisma;
	output["playerLevel"] = creatureLevel;
	output["gold"] = gold;
	output["gender"] = gender;
	output["naturalAttack"] = naturalAttack;
	output["ethics"] = encode_ethics(ethics);
	output["morality"] = encode_morality(morality);
	output["undead"] = undead;
	output["awareUntilTime"] = awareUntilTime;
	output["attitude"] = encode_attitude(attitude);
	output["displaceable"] = displaceable;
	output["morale"] = morale;
	output["corpseWeight"] = corpseWeight;
	output["corpseName"] = corpseName;
	output["corpseNutrition"] = corpseNutrition;
	output["corpseTaste"] = corpseTaste;
	output["corpseEdible"] = is_corpse_edible(corpseEdibility);
	json slotNames = json::array();
	for (const EquipmentSlot slot : bodyPlan)
	{
		slotNames.push_back(encode_equipment_slot(slot));
	}
	output["bodyPlan"] = slotNames;
	if (airUntilTime.has_value())
	{
		output["airUntilTime"] = *airUntilTime;
	}
	output["webStuckTurns"] = webStuckTurns;
	output["webStrength"] = webStrength;
	output["creatureClass"] = encode_creature_class(creatureClass);
	output["hitDie"] = hitDie;
	output["attacksPerRound"] = attacksPerRound;
	output["actionDelay"] = actionDelay;
	output["lifespan"] = lifespan.has_value()
		? json{
			  { "race", static_cast<int>(lifespan->race) },
			  { "startingAge", lifespan->startingAge },
			  { "maximumAge", lifespan->maximumAge },
			  { "yearsLived", lifespan->yearsLived },
			  { "magicalYears", lifespan->magicalYears }
		  }
		: json(nullptr);
	output["nextActionTime"] = nextActionTime;
	output["dr"] = damageResistance;
	output["thaco"] = thaco;
	if (pendingPoison.has_value())
	{
		json poisonJson;
		poisonJson["roundsUntilOnset"] = pendingPoison->roundsUntilOnset;
		poisonJson["damage"] = pendingPoison->damage;
		poisonJson["effect"] = encode_buff_type(pendingPoison->effect);
		poisonJson["effectValue"] = pendingPoison->effectValue;
		poisonJson["effectRounds"] = pendingPoison->effectRounds;
		output["pendingPoison"] = poisonJson;
	}
	if (attacker)
	{
		json attackerJson;
		attacker->save(attackerJson);
		// Only write the key if the strategy produced content.
		// PlayerAttacker::save() is a no-op - no damageInfo to persist.
		if (!attackerJson.empty())
		{
			output["attacker"] = attackerJson;
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
		healthJson["unregenerableDamage"] = healthPool->get_unregenerable_damage();
		output["healthPool"] = healthJson;
	}
	// Save constitution tracker state
	json constJson;
	if (const auto lastConstitution = get_last_constitution(); lastConstitution.has_value())
	{
		constJson["lastConstitution"] = *lastConstitution;
	}
	output["constitutionTracker"] = constJson;
	// Save experience reward
	if (experienceReward)
	{
		json expJson;
		experienceReward->save(expJson);
		output["experienceReward"] = expJson;
	}
	// Save armor class
	if (armorClass)
	{
		json acJson;
		acJson["armorClass"] = armorClass->get_armor_class();
		acJson["baseArmorClass"] = armorClass->get_base_armor_class();
		output["armorClass"] = acJson;
	}
	if (ai)
	{
		json aiJson;
		ai->save(aiJson);
		output["ai"] = aiJson;
	}
	// Always save inventory data since it always exists
	json inventoryJson;
	InventoryOperations::save_inventory(inventoryData, inventoryJson);
	output["inventoryData"] = inventoryJson;
	if (shop)
	{
		json shopJson;
		shop->save(shopJson);
		output["shop"] = shopJson;
	}

	// Save unified buff system
	json buffsJson = json::array();
	for (const auto& buff : activeBuffs)
	{
		json buffJson;
		buffJson["type"] = encode_buff_type(buff.type);
		buffJson["value"] = buff.value;
		buffJson["expiryTime"] = buff.expiryTime.has_value() ? json(*buff.expiryTime) : json(nullptr);
		buffJson["application"] = encode_buff_application(buff.application);
		json savesJson = json::array();
		for (const OpponentSave& save : buff.opponentSaves)
		{
			savesJson.push_back(json{ { "opponent", save.opponent }, { "isMade", save.isMade } });
		}
		buffJson["opponentSaves"] = savesJson;
		buffsJson.push_back(buffJson);
	}
	output["activeBuffs"] = buffsJson;

	json equippedJson = json::array();
	for (const EquippedItem& equipped : equippedItems)
	{
		assert(equipped.item && "Creature::save: an equipment entry holds no item");
		json itemJson;
		equipped.item->save(itemJson);
		equippedJson.push_back(json{ { "slot", encode_equipment_slot(equipped.slot) }, { "item", itemJson } });
	}
	output["equippedItems"] = equippedJson;
}

void Creature::update_creature_state(GameContext& ctx)
{
	assert(ctx.tileConfig && "Creature::update_creature_state: called without a tileConfig");
	if (!invisibleTile.is_valid())
	{
		invisibleTile = ctx.tileConfig->get("TILE_INVISIBLE");
	}

	assert(ctx.buffSystem && "update_creature_state: a creature was updated with no buff system");
	assert(ctx.gameState && "update_creature_state: a creature was updated with no clock to read its buffs against");

	// It reads the clock rather than counting, so running it once per action costs
	// nothing: a creature acting twice in a round reaches the same answer twice.
	const int currentTime = ctx.gameState->get_time();
	ctx.buffSystem->update_creature_buffs(*this, currentTime);

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

// One of the six, chosen at run time. The named setters stay and do the work;
// these are the entry points for code that does not know which ability it holds.
void Creature::set_ability(Ability ability, int value) noexcept
{
	switch (ability)
	{
	case Ability::STRENGTH:
	{
		set_strength(value);
		break;
	}
	case Ability::DEXTERITY:
	{
		set_dexterity(value);
		break;
	}
	case Ability::CONSTITUTION:
	{
		set_constitution(value);
		break;
	}
	case Ability::INTELLIGENCE:
	{
		set_intelligence(value);
		break;
	}
	case Ability::WISDOM:
	{
		set_wisdom(value);
		break;
	}
	case Ability::CHARISMA:
	{
		set_charisma(value);
		break;
	}
	}
}

void Creature::adjust_ability(Ability ability, int delta) noexcept
{
	switch (ability)
	{
	case Ability::STRENGTH:
	{
		adjust_strength(delta);
		break;
	}
	case Ability::DEXTERITY:
	{
		adjust_dexterity(delta);
		break;
	}
	case Ability::CONSTITUTION:
	{
		adjust_constitution(delta);
		break;
	}
	case Ability::INTELLIGENCE:
	{
		adjust_intelligence(delta);
		break;
	}
	case Ability::WISDOM:
	{
		adjust_wisdom(delta);
		break;
	}
	case Ability::CHARISMA:
	{
		adjust_charisma(delta);
		break;
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
	assert(ctx.dataManager && "Creature::update_constitution_bonus: called without a dataManager");
	const std::optional<int> oldCon = get_last_constitution();
	const auto result = constitutionTracker->apply_constitution_changes(*this, *ctx.dataManager);

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
			ctx.messageSystem->message(ColorPairId::GREEN_BLACK,
				std::format("Constitution increased from {} to {}! You gain {} hit points.", *oldCon, get_constitution(), result.hpDifference),
				MessageCompletion::FINISHED);
		}
		else
		{
			ctx.messageSystem->message(ColorPairId::RED_BLACK,
				std::format("Constitution decreased from {} to {}! You lose {} hit points.", *oldCon, get_constitution(), -result.hpDifference),
				MessageCompletion::FINISHED);
		}
	}

	if (get_hp() <= 0)
	{
		set_hp(0);
		if (is_player())
		{
			ctx.messageSystem->message(ColorPairId::RED_BLACK, "Your life force has been drained beyond recovery. You die!", MessageCompletion::FINISHED);
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
void Creature::regenerate_from_constitution(int roundsElapsed, const DataManager& dataManager)
{
	assert(roundsElapsed > 0 && "Creature::regenerate_from_constitution called before a round has run");

	// Every creature mends by Constitution, monsters included (DMG, Ability Scores for Monsters);
	// the column holds 0 below Constitution 20, the book's "Nil".
	const int turnsBetweenPoints = dataManager.constitution_for(get_constitution()).Regeneration;
	if (turnsBetweenPoints == 0)
	{
		return;
	}

	// A point each time the listed number of turns has run, reckoned in rounds.
	if (roundsElapsed % (turnsBetweenPoints * GameBalance::Time::ROUNDS_PER_TURN) == 0)
	{
		[[maybe_unused]] const int healed = healthPool->regenerate(1);
	}
}

void Creature::take_poison(PendingPoison dose)
{
	pendingPoison = dose;
}

void Creature::tick_poison(int roundEndTime, GameContext& ctx)
{
	if (!pendingPoison.has_value() || is_dead())
	{
		return;
	}

	--pendingPoison->roundsUntilOnset;
	if (pendingPoison->roundsUntilOnset > 0)
	{
		return;
	}

	// The dose is spent as it lands, so it is taken before the damage, which may kill.
	const int damage = pendingPoison->damage;
	const BuffType effect = pendingPoison->effect;
	const int effectValue = pendingPoison->effectValue;
	const int effectRounds = pendingPoison->effectRounds;
	pendingPoison.reset();

	// A poison that debilitates leaves its condition behind; one that wounds does not.
	if (effect != BuffType::NONE)
	{
		ctx.buffSystem->add_buff(*this, effect, effectValue, effectRounds, BuffApplication::ADD, roundEndTime);

		// The onset is a round after the bite, so it is announced when it arrives.
		ctx.messageSystem->message(actorData.color, actorData.name, MessageCompletion::CONTINUED);
		ctx.messageSystem->message(ColorPairId::WHITE_RED, " is weakened by the venom!", MessageCompletion::FINISHED);
	}

	if (damage <= 0)
	{
		return;
	}

	ctx.messageSystem->message(actorData.color, actorData.name, MessageCompletion::CONTINUED);
	ctx.messageSystem->message(ColorPairId::WHITE_RED, " is racked by the venom!", MessageCompletion::FINISHED);
	take_damage_and_check_death(damage, ctx, DamageType::POISON);
}

void Creature::regenerate_from_ring(int roundsElapsed)
{
	assert(roundsElapsed > 0 && "Creature::regenerate_from_ring called before a round has run");

	if (is_dead() || !wears_ring_of(MagicalEffect::REGENERATION))
	{
		return;
	}

	// A point each turn, reckoned in rounds; heal reaches fire and acid, which the ring mends.
	if (roundsElapsed % GameBalance::Time::ROUNDS_PER_TURN == 0)
	{
		[[maybe_unused]] const int healed = healthPool->heal(1);
	}
}

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

	assert(ctx.floatingText && "take_damage_hit_points: damage was taken with no floating text to show it");
	ctx.floatingText->spawn_damage(
		position,
		actualDamage,
		is_player() ? DamageSubject::PLAYER : DamageSubject::MONSTER);

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
	assert(ctx.map && "Creature::update_awareness: called without a map");
	assert(ctx.gameState && "update_awareness: a creature looked for the player with no clock to remember by");

	// An invisible player is not seen even in plain line of sight.
	if (ctx.map->is_in_fov(position) && !ctx.player()->is_invisible())
	{
		awareUntilTime = expiry_time(ctx.gameState->get_time(), AWARENESS_ROUNDS);
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

// Puts an item into a slot this creature's body provides. The creature owns it from
// here until it is taken off; a body without the slot refuses, because an item put
// where no lookup reaches is an item the wearer has lost.
//
// Example:
//   orc.wear(ItemCreator::create("long_sword", pos, ctx), EquipmentSlot::RIGHT_HAND);
//   orc.get_attack_name();  // -> "long sword"
//   wolf.wear(sword, EquipmentSlot::RIGHT_HAND);  // -> asserts: no such slot
void Creature::wear(std::unique_ptr<Item> item, EquipmentSlot slot)
{
	assert(item && "Creature::wear called with no item");
	assert(has_slot(slot) && "Creature::wear called with a slot this body does not have");

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

// Any worn item whose behaviour names an effect answers here, so an effect moves
// between slots - a ring, a helm, a pair of gauntlets - without this changing.
bool Creature::wears_item_with(MagicalEffect effect) const noexcept
{
	const auto grants_the_effect = [effect](const EquippedItem& equipped)
	{
		if (!equipped.item->behavior)
		{
			return false;
		}

		const auto matches = [effect](const auto& behavior)
		{
			if constexpr (GrantsMagicalEffect<std::remove_cvref_t<decltype(behavior)>>)
			{
				return behavior.effect == effect;
			}
			else
			{
				return false;
			}
		};
		return std::visit(matches, *equipped.item->behavior);
	};
	return std::ranges::any_of(equippedItems, grants_the_effect);
}

// A creature's own state, or the one worn source the game has for one: the gauntlets
// of swimming and climbing, whose wearer "can swim as fast as a triton" (Dungeon
// Master Guide, PDF page 964) and so crosses the water this game models swimming as.
// A creature that lives in water breathes it; one that is only visiting does not. The
// gauntlets of swimming carry a wearer across water and say nothing about air, which is
// the gap the helm of underwater action fills.
bool Creature::needs_air_underwater() const noexcept
{
	if (has_state(ActorState::CAN_SWIM))
	{
		return false;
	}
	return !wears_item_with(MagicalEffect::UNDERWATER_ACTION);
}

void Creature::tick_breath(int roundEndTime, GameContext& ctx)
{
	assert(ctx.map && "tick_breath: a round ran with no map to stand on");
	assert(ctx.dice && "tick_breath: a round ran with no dice to check against");

	// Dry ground, flight over water, water breathing or the helm: no breath is held, and any held is over.
	if (!ctx.map->is_water(position) || is_flying() || !needs_air_underwater())
	{
		airUntilTime.reset();
		return;
	}

	// The round it is first found in water is the gulp of air.
	if (!airUntilTime.has_value())
	{
		airUntilTime = expiry_time(roundEndTime, breath_rounds(get_constitution()));
		return;
	}

	if (roundEndTime < *airUntilTime)
	{
		return;
	}

	// Past the allowance. The penalty comes off the clock rather than a second counter:
	// how far past it is already says how many checks have been made.
	const int roundsPast = rounds_completed_at(roundEndTime - *airUntilTime);
	const int target = get_constitution() + breath_check_penalty(roundsPast);

	if (ctx.dice->d20() <= target)
	{
		return;
	}

	ctx.messageSystem->append_message_part(actorData.color, std::format("{}", actorData.name));
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " cannot hold out any longer and drowns!\n");
	ctx.messageSystem->finalize_message();

	// die() reads the health pool rather than emptying it, so drowning empties it first.
	set_hp(0);
	die(ctx);
}

bool Creature::has_bypass(ActorState state) const noexcept
{
	if (has_state(state))
	{
		return true;
	}
	if (state != ActorState::CAN_SWIM)
	{
		return false;
	}

	// Either item carries a wearer across water: the gauntlets of swimming, and the
	// helm of underwater action, which also keeps the air on once they are in.
	return wears_item_with(MagicalEffect::SWIMMING) || wears_item_with(MagicalEffect::UNDERWATER_ACTION);
}

bool Creature::wears_ring_of(MagicalEffect effect) const noexcept
{
	auto holds_ring_of_effect = [this, effect](EquipmentSlot hand)
	{
		const Item* ring = get_equipped_item(hand);
		const MagicalRing* magicalRing = ring && ring->behavior ? std::get_if<MagicalRing>(&*ring->behavior) : nullptr;
		return magicalRing != nullptr && magicalRing->effect == effect;
	};
	return holds_ring_of_effect(EquipmentSlot::RIGHT_RING) || holds_ring_of_effect(EquipmentSlot::LEFT_RING);
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
	assert(ctx.dice && "Creature::try_break_web: called without dice");

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

void Creature::apply_confusion(int durationRounds, int currentTime)
{
	ai = std::make_unique<AiMonsterConfused>(expiry_time(currentTime, durationRounds), std::move(ai));
}

void Creature::drop(Item& item, GameContext& ctx)
{
	assert(ctx.floorInventory && "Creature::drop: called without a floorInventory");

	[[maybe_unused]] auto is_null = [](const auto& invItem)
	{
		return !invItem;
	};
	assert(std::ranges::none_of(inventoryData.items, is_null) && "Creature::drop: the pack holds a null item");

	auto matches_item = [&item](const auto& invItem)
	{
		return invItem.get() == &item;
	};
	auto matches = inventoryData.items | std::views::filter(matches_item);

	if (std::ranges::empty(matches))
	{
		return;
	}

	// The floor has no limit, so a drop always lands.
	auto& foundPtr = matches.front();
	foundPtr->position = position;
	InventoryOperations::add_item(*ctx.floorInventory, std::move(foundPtr));
	InventoryOperations::optimize_inventory_storage(inventoryData);
	ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You dropped the item.", MessageCompletion::FINISHED);
}

bool Creature::equip_item(std::unique_ptr<Item> item, EquipmentSlot slot, GameContext& ctx)
{
	if (!item)
	{
		if (ctx.messageSystem->is_debug_mode())
		{
			ctx.messageSystem->log("DEBUG: equip_item failed - null item");
		}
		return false;
	}

	if (!can_equip(*item, slot))
	{
		if (ctx.messageSystem->is_debug_mode())
		{
			ctx.messageSystem->log("DEBUG: can_equip failed for " + item->get_name());
			ctx.messageSystem->log("DEBUG: Item class: " + std::to_string(static_cast<int>(item->itemClass)));
			ctx.messageSystem->log("DEBUG: Is armor: " + std::string(item->is_armor() ? "true" : "false"));
			ctx.messageSystem->log("DEBUG: Slot: " + std::to_string(static_cast<int>(slot)));
			ctx.messageSystem->log("DEBUG: equip_item failed - can_equip returned false for " + item->get_name() + " in slot " + std::to_string(static_cast<int>(slot)));
		}
		// Back to the pack it came out of: carrying it again adds no weight, so no weight gate
		[[maybe_unused]] const auto pickUpItemResult = InventoryOperations::add_item(inventoryData, std::move(item));
		assert(pickUpItemResult.has_value() && "Creature::equip_item: an item the slot refused could not go back to the pack");
		return false;
	}

	// A weapon made for a stronger arm cannot be drawn by this one: the first Baldur's
	// Gate's composite bow "Requires: 18 Strength".
	if (!can_draw(*this, *item))
	{
		ctx.messageSystem->message(
			ColorPairId::WHITE_BLACK,
			std::format("You are not strong enough to draw the {}.", item->get_name()),
			MessageCompletion::FINISHED);
		// Back to the pack it came out of, as for a slot that refuses it
		[[maybe_unused]] const auto returnedToPack = InventoryOperations::add_item(inventoryData, std::move(item));
		assert(returnedToPack.has_value() && "Creature::equip_item: a bow too strong to draw could not go back to the pack");
		return false;
	}

	// Unequip existing item in the slot first
	unequip_item(slot, ctx);

	// Special handling for two-handed weapons - use ItemClass system
	if (slot == EquipmentSlot::RIGHT_HAND)
	{
		if (item->is_two_handed_weapon())
		{
			// Two-handed weapon - also unequip left hand
			unequip_item(EquipmentSlot::LEFT_HAND, ctx);
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You grip the " + item->get_name() + " with both hands.", MessageCompletion::FINISHED);
		}
	}

	// Add equipped item
	equippedItems.emplace_back(std::move(item), slot);

	// Log weapon equip
	if (slot == EquipmentSlot::RIGHT_HAND && equippedItems.back().item->is_weapon())
	{
		std::string weaponDamage = WeaponDamageRegistry::get_damage_roll(equippedItems.back().item->itemKey);
		ctx.messageSystem->log("Equipped " + equippedItems.back().item->get_name() + " - damage: " + weaponDamage);
	}

	// Update armor class if armor or shield was equipped
	if (slot == EquipmentSlot::BODY || slot == EquipmentSlot::LEFT_HAND)
	{
		update_armor_class(ctx);
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Your armor class is now " + std::to_string(get_armor_class()) + ".", MessageCompletion::FINISHED);
	}

	return true;
}

bool Creature::unequip_item(EquipmentSlot slot, GameContext& ctx)
{
	assert(ctx.dataManager && "Creature::unequip_item: called without a dataManager");
	assert(ctx.floorInventory && "Creature::unequip_item: called without a floorInventory");

	// erase after move requires iterator; find_if + erase is the correct C++23 pattern here
	auto found = std::ranges::find_if(equippedItems, matches_slot(slot));

	if (found != equippedItems.end())
	{
		// AD&D 2e PHB p.230: cursed items are magically bound to the wearer.
		// Only a Remove Curse spell breaks the bond.
		if (found->item->get_enhancement().blessing == BlessingStatus::CURSED)
		{
			ctx.messageSystem->message(
				ColorPairId::RED_BLACK,
				std::format("The {} is cursed and cannot be removed!", found->item->get_name()),
				MessageCompletion::FINISHED);
			return false;
		}

		const int armorClassBefore = get_armor_class();

		// Out of the slot list before anything reads it again: the pack's weight check reads
		// Strength, which counts every worn item.
		std::unique_ptr<Item> removed = std::move(found->item);
		equippedItems.erase(found);
		make_dormant(*removed);

		// The pack takes it if it has room and weight to spare; otherwise it drops at the wearer's feet,
		// since taking off a Strength item can lower the limit, and the floor always has room.
		const bool fitsInPack = !InventoryOperations::is_inventory_full(inventoryData) && InventoryOperations::is_within_weight_limit(*removed, *this, *ctx.dataManager);

		if (fitsInPack)
		{
			[[maybe_unused]] const auto packed = InventoryOperations::add_item_to_inventory(inventoryData, std::move(removed), *this, *ctx.dataManager);
			assert(packed.has_value() && "Creature::unequip_item: the pack refused an item checked to fit");
		}
		else
		{
			ctx.messageSystem->message(
				ColorPairId::WHITE_BLACK,
				std::format("You cannot carry the {} as well, and set it down.", removed->get_name()),
				MessageCompletion::FINISHED);
			removed->position = position;
			InventoryOperations::add_item(*ctx.floorInventory, std::move(removed));
		}

		// Any worn item can carry armour class, so it is recomputed whatever slot was emptied.
		update_armor_class(ctx);
		if (get_armor_class() != armorClassBefore)
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Your armor class is now " + std::to_string(get_armor_class()) + ".", MessageCompletion::FINISHED);
		}

		return true;
	}

	return false;
}

bool Creature::is_slot_occupied(EquipmentSlot slot) const noexcept
{
	return get_equipped_item(slot) != nullptr;
}

bool Creature::is_item_equipped(uint64_t itemUniqueId) const noexcept
{
	return std::ranges::any_of(equippedItems, matches_unique_id(itemUniqueId));
}

bool Creature::is_dual_wielding() const noexcept
{
	// Check if both hands have weapons equipped
	auto rightHand = get_equipped_item(EquipmentSlot::RIGHT_HAND);
	auto leftHand = get_equipped_item(EquipmentSlot::LEFT_HAND);

	if (!rightHand || !leftHand)
	{
		return false;
	}

	// Check if left hand item is a weapon (not a shield) - use ItemClass system
	if (leftHand->is_shield())
	{
		return false; // Shield, not dual wielding
	}

	// Check if left hand item is a weapon
	if (leftHand->is_weapon())
	{
		return true; // Both hands have weapons
	}

	return false;
}

// Clean Equipment System using Unique IDs
bool Creature::toggle_armor(uint64_t itemUniqueId, GameContext& ctx)
{
	// Check if item is already equipped
	if (is_item_equipped(itemUniqueId))
	{
		// Unequip the armor
		return unequip_item(EquipmentSlot::BODY, ctx);
	}
	else
	{
		auto result = InventoryOperations::remove_item_by_id(inventoryData, itemUniqueId);
		if (result.has_value())
		{
			return equip_item(std::move(*result), EquipmentSlot::BODY, ctx);
		}
		return false;
	}
}

bool Creature::toggle_shield(uint64_t itemUniqueId, GameContext& ctx)
{
	// Shields always go in LEFT_HAND slot
	if (is_item_equipped(itemUniqueId))
	{
		return unequip_item(EquipmentSlot::LEFT_HAND, ctx);
	}
	else
	{
		auto result = InventoryOperations::remove_item_by_id(inventoryData, itemUniqueId);
		if (result.has_value())
		{
			return equip_item(std::move(*result), EquipmentSlot::LEFT_HAND, ctx);
		}
		return false;
	}
}

// Clean Weapon Equipment System using Unique IDs
bool Creature::toggle_weapon(uint64_t itemUniqueId, EquipmentSlot preferredSlot, GameContext& ctx)
{
	// Check if weapon is already equipped
	if (is_item_equipped(itemUniqueId))
	{
		// Find which slot and unequip
		for (auto slot : { EquipmentSlot::RIGHT_HAND, EquipmentSlot::LEFT_HAND, EquipmentSlot::MISSILE_WEAPON })
		{
			Item* equipped = get_equipped_item(slot);
			if (equipped && equipped->uniqueId == itemUniqueId)
			{
				return unequip_item(slot, ctx);
			}
		}
	}
	else
	{
		// Take the item out of the pack; an id the pack does not hold equips nothing.
		auto result = InventoryOperations::remove_item_by_id(inventoryData, itemUniqueId);
		if (result.has_value())
		{
			auto itemToEquip = std::move(*result);

			// A ranged weapon goes to the missile slot whichever hand was asked for.
			const EquipmentSlot targetSlot = ItemClassificationUtils::is_ranged_weapon(itemToEquip->itemClass)
				? EquipmentSlot::MISSILE_WEAPON
				: preferredSlot;

			return equip_item(std::move(itemToEquip), targetSlot, ctx);
		}
	}
	return false;
}

bool Creature::toggle_equipment(uint64_t itemUniqueId, EquipmentSlot slot, GameContext& ctx)
{
	// Check if item is already equipped
	if (is_item_equipped(itemUniqueId))
	{
		// Unequip the item
		return unequip_item(slot, ctx);
	}
	else
	{
		auto result = InventoryOperations::remove_item_by_id(inventoryData, itemUniqueId);
		if (result.has_value())
		{
			return equip_item(std::move(*result), slot, ctx);
		}
		return false;
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

ColorPairId Creature::get_display_color() const noexcept
{
	if (is_invisible())
	{
		return ColorPairId::CYAN_BLACK;
	}
	return Actor::get_display_color();
}

void Creature::die(GameContext& ctx)
{
	assert(ctx.tileConfig && "Creature::die: called without a tileConfig");
	assert(ctx.floorInventory && "Creature::die: called without a floorInventory");

	// Monster death: message, reward, animation, drop items, create corpse
	ctx.messageSystem->append_message_part(actorData.color, std::format("{}", actorData.name));
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " is dead.\n");
	ctx.messageSystem->finalize_message();

	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "You get ");
	ctx.messageSystem->append_message_part(ColorPairId::YELLOW_BLACK, std::format("{}", get_xp()));
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, " experience points.\n");
	ctx.messageSystem->finalize_message();

	assert(ctx.player() != nullptr && "Creature::die requires a live player in context");
	ctx.player_concrete().on_kill_reward(get_xp(), ctx);

	assert(ctx.animSystem && "Creature::die: called without an animSystem");
	ctx.animSystem->spawn_death(position);

	[[maybe_unused]] auto is_nothing = [](const auto& carried)
	{
		return !carried;
	};
	assert(std::ranges::none_of(inventoryData.items, is_nothing) && "a pack holds nothing where an item should be");
	for (auto& item : inventoryData.items)
	{
		item->position = position;
		InventoryOperations::add_item(*ctx.floorInventory, std::move(item));
	}
	inventoryData.items.clear();

	// What it wore falls with it, no longer worn. The floor has no limit, so nothing is lost.
	for (EquippedItem& equipped : equippedItems)
	{
		assert(equipped.item && "Creature::die: an equipment entry holds no item");
		equipped.item->position = position;
		InventoryOperations::add_item(*ctx.floorInventory, std::move(equipped.item));
	}
	equippedItems.clear();

	// Create a corpse on the floor in place of the creature, as its record describes it.
	assert(!corpseName.empty() && "Creature::die: the creature was given no corpse to leave");
	auto corpse = std::make_unique<Item>(position, actorData);
	corpse->actorData.name = corpseName;
	corpse->actorData.tile = ctx.tileConfig->get("TILE_CORPSE");
	corpse->enhancement.weight = get_corpse_weight();
	corpse->behavior = CorpseFood{ corpseNutrition, corpseTaste, corpseEdibility };
	InventoryOperations::add_item(*ctx.floorInventory, std::move(corpse));
}

int Creature::get_exceptional_strength() const noexcept
{
	// The creature's own percentile, after what age has taken from it.
	int exceptional = lifespan ? aged_exceptional_strength(*lifespan, exceptionalStrength) : exceptionalStrength;

	// A worn item that sets Strength to 18 brings its own percentile; the higher counts.
	auto from_setting_item = [&exceptional](const auto& boost)
	{
		using BoostType = std::decay_t<decltype(boost)>;
		if constexpr (std::is_same_v<BoostType, JewelryAmulet> || std::is_same_v<BoostType, Gauntlets> || std::is_same_v<BoostType, Girdle>)
		{
			if (boost.application == BuffApplication::SET && boost.strBonus == 18)
			{
				exceptional = std::max(exceptional, boost.exceptionalStrength);
			}
		}
	};
	for (const EquippedItem& worn : equippedItems)
	{
		assert(worn.item && "get_exceptional_strength: an equipment slot holds a null item");
		if (worn.item->behavior)
		{
			std::visit(from_setting_item, *worn.item->behavior);
		}
	}
	return exceptional;
}

const Gauntlets* Creature::get_gauntlets_beside_girdle() const noexcept
{
	bool wearsGiantStrength = false;
	const Gauntlets* strengthGauntlets = nullptr;
	for (const EquippedItem& worn : equippedItems)
	{
		assert(worn.item && "get_gauntlets_beside_girdle: an equipment slot holds a null item");
		wearsGiantStrength = wearsGiantStrength || is_giant_strength_girdle(*worn.item);
		if (const Gauntlets* gauntlets = strength_setting_gauntlets(*worn.item))
		{
			strengthGauntlets = gauntlets;
		}
	}
	return wearsGiantStrength ? strengthGauntlets : nullptr;
}

namespace
{
// Says so when an age has moved into a later band of Table 12.
//
// Example, a human going from 44 to 45:
//   announce_new_band(AgeCategory::YOUNG, AgeCategory::MIDDLE_AGE, ctx);   // "You feel the years: middle age."
void announce_new_band(AgeCategory before, AgeCategory after, GameContext& ctx)
{
	if (after <= before)
	{
		return;
	}
	ctx.messageSystem->message(ColorPairId::YELLOW_BLACK, std::format("You feel the years: {}.", age_category_name(after)), MessageCompletion::FINISHED);
}

// The ability score a buff type raises or lowers, or nothing for a buff that is not one.
//
// Example:
//   ability_of_buff(BuffType::WISDOM);   // -> Ability::WISDOM
//   ability_of_buff(BuffType::SPEED);    // -> std::nullopt
std::optional<Ability> ability_of_buff(BuffType type)
{
	if (type == BuffType::STRENGTH)
	{
		return Ability::STRENGTH;
	}
	if (type == BuffType::DEXTERITY)
	{
		return Ability::DEXTERITY;
	}
	if (type == BuffType::CONSTITUTION)
	{
		return Ability::CONSTITUTION;
	}
	if (type == BuffType::INTELLIGENCE)
	{
		return Ability::INTELLIGENCE;
	}
	if (type == BuffType::WISDOM)
	{
		return Ability::WISDOM;
	}
	if (type == BuffType::CHARISMA)
	{
		return Ability::CHARISMA;
	}
	return std::nullopt;
}
} // namespace

void Creature::add_magical_years(int years, GameContext& ctx)
{
	assert(lifespan && "add_magical_years: a creature with no lifespan was aged");
	assert(ctx.messageSystem && "add_magical_years: called without a messageSystem");

	// The band is read on both sides of the change, so nothing stores it.
	const AgeCategory before = age_category(lifespan->physical_age(), lifespan->race);
	lifespan->magicalYears += years;
	announce_new_band(before, age_category(lifespan->physical_age(), lifespan->race), ctx);
}

void Creature::pass_a_year(GameContext& ctx)
{
	assert(lifespan && "pass_a_year: a year passed for a creature with no lifespan");
	assert(lifespan->race != PlayerRaceState::NONE && "pass_a_year: a year passed for a character with no race");
	assert(ctx.messageSystem && "pass_a_year: called without a messageSystem");

	const AgeCategory before = age_category(lifespan->physical_age(), lifespan->race);
	++lifespan->yearsLived;
	announce_new_band(before, age_category(lifespan->physical_age(), lifespan->race), ctx);

	// Only time spends the life span, so the natural age is the one that reaches it.
	if (lifespan->natural_age() >= lifespan->maximumAge)
	{
		ctx.messageSystem->message(ColorPairId::RED_BLACK, "You die of old age.", MessageCompletion::FINISHED);
		die(ctx);
	}
}

int Creature::get_action_delay() const noexcept
{
	auto is_speed = [](const Buff& activeBuff)
	{
		return activeBuff.type == BuffType::SPEED;
	};
	if (!std::ranges::any_of(activeBuffs, is_speed))
	{
		return actionDelay;
	}

	// Twice the actions in the same time is half the time each.
	const int hastedDelay = actionDelay / 2;
	assert(hastedDelay >= 1 && "get_action_delay: a base delay too small to halve");
	return hastedDelay;
}

// AD&D 2e: an ability score is the creature's own, or the highest value any buff or worn
// item sets it to if that is higher, plus every buff's and worn item's addition. Under a
// girdle of giant strength, Strength takes the penalties and none of the bonuses, since
// its Strength "is not cumulative with normal or magical Strength bonuses" (DMG, PDF page
// 966). Nothing is written back, so what is removed takes exactly its own contribution
// with it.
int Creature::calculate_effective_stat(int base_value, BuffType type) const noexcept
{
	auto matchesType = [type](const Buff& activeBuff)
	{
		return activeBuff.type == type;
	};
	auto matchingBuffs = activeBuffs | std::views::filter(matchesType);

	int highestSet = 0; // Highest SET effect (Potion of Giant Strength -> 18)
	int bonuses = 0; // Additions above zero (Gauntlets +2)
	int penalties = 0; // Additions below zero (a weapon of weakness, -1)
	auto count_addition = [&bonuses, &penalties](int amount)
	{
		if (amount > 0)
		{
			bonuses += amount;
		}
		else
		{
			penalties += amount;
		}
	};

	for (const auto& buff : matchingBuffs)
	{
		if (buff.application == BuffApplication::SET)
		{
			highestSet = std::max(highestSet, buff.value);
		}
		else
		{
			count_addition(buff.value);
		}
	}

	// Worn items count exactly as buffs do.
	bool wearsGiantStrength = false;
	for (const EquippedItem& worn : equippedItems)
	{
		assert(worn.item && "calculate_effective_stat: an equipment slot holds a null item");
		const WornAbilityEffect effect = worn_ability_effect(*worn.item, type);
		highestSet = std::max(highestSet, effect.setTo);
		count_addition(effect.add);
		wearsGiantStrength = wearsGiantStrength || is_giant_strength_girdle(*worn.item);
	}

	// Ageing counts as an addition, read from the lifespan on every call.
	if (lifespan)
	{
		if (const std::optional<Ability> ability = ability_of_buff(type))
		{
			count_addition(ageing_change(*lifespan, *ability, exceptionalStrength));
		}
	}

	// AD&D 2e: SET effects replace base (if higher), ADD effects stack
	// Example: base=14, SET=18, ADD=+2 -> MAX(14,18) + 2 = 20
	int effectiveBase = (highestSet > 0) ? std::max(base_value, highestSet) : base_value;
	// A girdle of giant strength refuses the bonuses and keeps the penalties.
	const bool takesBonuses = type != BuffType::STRENGTH || !wearsGiantStrength;
	return effectiveBase + (takesBonuses ? bonuses : 0) + penalties;
}