// file: Destructible.cpp
#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "../Actor/Creature.h"
#include "../Actor/InventoryOperations.h"
#include "../Attributes/ConstitutionAttributes.h"
#include "../Attributes/DexterityAttributes.h"
#include "../Colors/Colors.h"
#include "../Combat/DamageInfo.h"
#include "../Core/GameContext.h"
#include "../Persistent/Persistent.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/BuffSystem.h"
#include "../Systems/BuffType.h"
#include "../Systems/DataManager.h"
#include "../Systems/FloatingTextSystem.h"
#include "../Systems/GameStateManager.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/TileConfig.h"
#include "Destructible.h"
#include "EquipmentSlot.h"
#include "Pickable.h"

// OCP: Data-driven damage resistance mapping
static const std::unordered_map<DamageType, BuffType> damageResistanceBuffs = {
	{ DamageType::FIRE, BuffType::FIRE_RESISTANCE },
	{ DamageType::COLD, BuffType::COLD_RESISTANCE },
	{ DamageType::LIGHTNING, BuffType::LIGHTNING_RESISTANCE },
	{ DamageType::POISON, BuffType::POISON_RESISTANCE },
	// DamageType::PHYSICAL, ACID, MAGIC have no resistance buffs
};

// OCP: Data-driven damage type names for logging
static const std::unordered_map<DamageType, std::string_view> damageTypeNames = {
	{ DamageType::PHYSICAL, "physical" },
	{ DamageType::FIRE, "fire" },
	{ DamageType::COLD, "cold" },
	{ DamageType::LIGHTNING, "lightning" },
	{ DamageType::POISON, "poison" },
	{ DamageType::ACID, "acid" },
	{ DamageType::MAGIC, "magic" },
};

Destructible::Destructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass)
	: hpBase(hpMax),
	  hpMax(hpMax),
	  hp(hpMax),
	  dr(dr),
	  corpseName(corpseName),
	  xp(xp),
	  thaco(thaco),
	  armorClass(armorClass),
	  baseArmorClass(armorClass),
	  lastConstitution(0),
	  tempHp(0)
{
}

void Destructible::update_constitution_bonus(Creature& owner, GameContext& ctx)
{
	const int currentConstitution = owner.get_constitution();
	const int lastConstitution = get_last_constitution();

	if (currentConstitution == lastConstitution)
	{
		return;
	}

	const int oldBonus = calculate_constitution_hp_bonus_for_value(lastConstitution, ctx);
	const int newBonus = calculate_constitution_hp_bonus(owner, ctx);
	const int level = calculate_level_multiplier(owner);
	const int hpDifference = (newBonus - oldBonus) * level;

	if (hpDifference != 0)
	{
		// Constitution changes affect both max HP and current HP
		// Temporary HP are NOT affected by Constitution changes
		set_max_hp(get_max_hp() + hpDifference);
		set_hp(get_hp() + hpDifference);

		log_constitution_change(owner, ctx, lastConstitution, currentConstitution, hpDifference);

		// Check for stat drain death
		if (get_hp() <= 0)
		{
			set_hp(0);
			handle_stat_drain_death(owner, ctx);
		}
	}

	set_last_constitution(currentConstitution);
}

[[nodiscard]] int Destructible::calculate_constitution_hp_bonus(const Creature& owner, GameContext& ctx) const
{
	return calculate_constitution_hp_bonus_for_value(owner.get_constitution(), ctx);
}

[[nodiscard]] int Destructible::calculate_constitution_hp_bonus_for_value(int constitution, GameContext& ctx) const
{
	const auto& constitutionAttributes = ctx.dataManager->get_constitution_attributes();

	// Validate constitution is in valid range (1-based indexing)
	if (constitution < 1 || constitution > static_cast<int>(constitutionAttributes.size()))
	{
		return 0;
	}

	return constitutionAttributes[constitution - 1].HPAdj;
}

[[nodiscard]] int Destructible::calculate_level_multiplier(const Creature& owner) const
{
	// AD&D 2e: Polymorphic Constitution HP multiplier
	// - Monsters: Use Hit Dice (no cap)
	// - Players: Class-specific caps (Fighter=9, Priest=9, Rogue/Wizard=10)
	return owner.get_constitution_hp_multiplier();
}

void Destructible::handle_stat_drain_death(Creature& owner, GameContext& ctx)
{
	if (&owner == ctx.player)
	{
		ctx.messageSystem->message(RED_BLACK_PAIR, "Your life force has been drained beyond recovery. You die!", true);
	}
	else
	{
		ctx.messageSystem->log(std::format("{} dies from stat drain.", owner.get_name()));
	}

	// Trigger death system
	die(owner, ctx);
}

void Destructible::log_constitution_change(const Creature& owner, GameContext& ctx, int oldCon, int newCon, int hpChange) const
{
	// Only log for player
	if (&owner != ctx.player)
	{
		return;
	}

	if (hpChange > 0)
	{
		ctx.messageSystem->message(GREEN_BLACK_PAIR,
			std::format("Constitution increased from {} to {}! You gain {} hit points.", oldCon, newCon, hpChange),
			true);
	}
	else
	{
		ctx.messageSystem->message(RED_BLACK_PAIR,
			std::format("Constitution decreased from {} to {}! You lose {} hit points.", oldCon, newCon, -hpChange),
			true);
	}
}

int Destructible::take_damage(Creature& owner, int damage, GameContext& ctx, DamageType damageType)
{
	// Negative or zero damage has no effect
	if (damage <= 0)
	{
		return 0;
	}

	const int originalDamage = damage;

	// AD&D 2e: Apply resistance based on damage type (data-driven, OCP compliant)
	if (damageResistanceBuffs.contains(damageType))
	{
		const BuffType resistanceBuff = damageResistanceBuffs.at(damageType);
		if (ctx.buffSystem->has_buff(owner, resistanceBuff))
		{
			const int resistancePercent = ctx.buffSystem->get_buff_value(owner, resistanceBuff);

			if (resistancePercent > 0)
			{
				// AD&D 2e: Resistance reduces damage by percentage (validated: logic correct)
				const int damageReduced = (damage * resistancePercent) / 100;
				damage -= damageReduced;
				damage = std::max(0, damage); // Defensive: ensure non-negative

				// Log resistance message
				const std::string_view typeName = damageTypeNames.contains(damageType)
					? damageTypeNames.at(damageType)
					: "unknown";

				ctx.messageSystem->log(std::format(
					"You resisted {} {} damage! ({}% resistance, {} -> {})",
					damageReduced,
					typeName,
					resistancePercent,
					originalDamage,
					damage));
			}
		}
	}

	// AD&D 2e: Temp HP absorbs damage first (validated: logic correct)
	if (get_temp_hp() > 0)
	{
		const int tempAbsorbed = std::min(damage, get_temp_hp());
		set_temp_hp(get_temp_hp() - tempAbsorbed);
		damage -= tempAbsorbed;

		if (damage == 0)
		{
			return 0; // All damage absorbed by temp HP
		}
	}

	// Remaining damage hits real HP
	const int newHp = get_hp() - damage;
	set_hp(newHp);

	// Explicit clamping and death check
	if (get_hp() <= 0)
	{
		set_hp(0);
		die(owner, ctx);
	}

	// Floating damage number: red for player damage, yellow for monster damage
	if (ctx.floatingText)
	{
		bool hitPlayer = (&owner == ctx.player);
		unsigned char r = hitPlayer ? 255 : 255;
		unsigned char g = hitPlayer ? 80 : 220;
		unsigned char b = hitPlayer ? 80 : 50;
		ctx.floatingText->spawn_damage(owner.position.x, owner.position.y, damage, r, g, b);
	}

	return damage;
}

// Transform the actor into a corpse !
void Destructible::die(Creature& owner, GameContext& ctx)
{
	// copy data to new entity of type Item
	auto corpse = std::make_unique<Item>(owner.position, owner.actorData);
	corpse->actorData.name = get_corpse_name();
	corpse->actorData.tile = ctx.tileConfig->get("TILE_CORPSE");
	corpse->behavior = CorpseFood{ 0 }; // 0 means calculate from type

	// Add the corpse to the floor items
	InventoryOperations::add_item(*ctx.inventoryData, std::move(corpse));

	// Note: The creature will be removed from game->creatures later
	// when it's safe to do so (e.g., at the end of the turn)
}

//====

// The function returns the amount of health point actually restored.
[[nodiscard]] int Destructible::heal(int hpToHeal)
{
	const int currentHp = get_hp();
	const int maxHp = get_max_hp();
	const int newHp = std::min(currentHp + hpToHeal, maxHp);
	const int actualHealed = newHp - currentHp;

	set_hp(newHp);

	// Explicit clamping
	if (get_hp() > get_max_hp())
	{
		set_hp(get_max_hp());
	}

	return actualHealed;
}

void Destructible::update_armor_class(Creature& owner, GameContext& ctx)
{
	const int baseAC = get_base_armor_class();
	const int dexBonus = calculate_dexterity_ac_bonus(owner, ctx);
	const int equipBonus = calculate_equipment_ac_bonus(owner, ctx);
	const int tempBonus = ctx.buffSystem->calculate_ac_bonus(owner);
	const int calculatedAC = baseAC + dexBonus + equipBonus + tempBonus;

	// Only update if changed
	if (get_armor_class() != calculatedAC)
	{
		const int oldAC = get_armor_class();
		set_armor_class(calculatedAC);

		// Log for player
		if (&owner == ctx.player)
		{
			ctx.messageSystem->log(std::format(
				"Armor Class updated: {} → {} (Base: {}, Dex: {:+}, Equipment: {:+}, Temp: {:+})",
				oldAC,
				calculatedAC,
				baseAC,
				dexBonus,
				equipBonus,
				tempBonus));
		}
	}
}

[[nodiscard]] int Destructible::calculate_dexterity_ac_bonus(const Creature& owner, GameContext& ctx) const
{
	const auto& dexAttributes = ctx.dataManager->get_dexterity_attributes();
	const int dexterity = owner.get_dexterity();

	// Validate dexterity is in valid range (1-based indexing)
	if (dexterity <= 0 || dexterity > static_cast<int>(dexAttributes.size()))
	{
		return 0;
	}

	const int defensiveAdj = dexAttributes[dexterity - 1].DefensiveAdj;

	// Log for player if non-zero
	if (&owner == ctx.player && defensiveAdj != 0)
	{
		ctx.messageSystem->log(std::format(
			"Dexterity Defensive Adjustment: {:+} (Dex: {})",
			defensiveAdj,
			dexterity));
	}

	return defensiveAdj;
}

// Single source of truth: slot-based equipment AC calculation
// Works polymorphically via virtual get_equipped_item():
//   - Players: returns items from equipment slots
//   - NPCs: returns nullptr (no slot-based equipment, 0 AC bonus)
[[nodiscard]] int Destructible::calculate_equipment_ac_bonus(const Creature& owner, GameContext& ctx) const
{
	int totalBonus = 0;

	// Check body armor - polymorphic via virtual get_ac_bonus()
	if (Item* equippedArmor = owner.get_equipped_item(EquipmentSlot::BODY))
	{
		const int armorBonus = equippedArmor->behavior ? get_item_ac_bonus(*equippedArmor->behavior) : 0;
		if (armorBonus != 0)
		{
			totalBonus += armorBonus;

			if (&owner == ctx.player)
			{
				ctx.messageSystem->log(std::format(
					"Armor bonus: {:+} from {}",
					armorBonus,
					equippedArmor->actorData.name));
			}
		}
	}

	// Check shield in left hand - polymorphic via virtual get_ac_bonus()
	if (Item* equippedShield = owner.get_equipped_item(EquipmentSlot::LEFT_HAND))
	{
		const int shieldBonus = equippedShield->behavior ? get_item_ac_bonus(*equippedShield->behavior) : 0;
		if (shieldBonus != 0)
		{
			totalBonus += shieldBonus;

			if (&owner == ctx.player)
			{
				ctx.messageSystem->log(std::format(
					"Shield bonus: {:+} from {}",
					shieldBonus,
					equippedShield->actorData.name));
			}
		}
	}

	// Check rings for protection bonuses (AD&D 2e: best ring applies, no stacking)
	int bestRingBonus = 0;
	const Item* bestRing = nullptr;

	for (const auto slot : { EquipmentSlot::RIGHT_RING, EquipmentSlot::LEFT_RING })
	{
		if (Item* equippedRing = owner.get_equipped_item(slot))
		{
			const int ringBonus = equippedRing->behavior ? get_item_ac_bonus(*equippedRing->behavior) : 0;
			if (ringBonus < bestRingBonus)
			{
				bestRingBonus = ringBonus;
				bestRing = equippedRing;
			}
		}
	}

	if (bestRingBonus < 0 && bestRing)
	{
		totalBonus += bestRingBonus;

		if (&owner == ctx.player)
		{
			ctx.messageSystem->log(std::format(
				"Ring bonus: {:+} from {}",
				bestRingBonus,
				bestRing->actorData.name));
		}
	}

	// Check helm for AC bonuses - polymorphic via virtual get_ac_bonus()
	if (Item* equippedHelm = owner.get_equipped_item(EquipmentSlot::HEAD))
	{
		const int helmBonus = equippedHelm->behavior ? get_item_ac_bonus(*equippedHelm->behavior) : 0;
		if (helmBonus < 0)
		{
			totalBonus += helmBonus;

			if (&owner == ctx.player)
			{
				ctx.messageSystem->log(std::format(
					"Helm bonus: {:+} from {}",
					helmBonus,
					equippedHelm->actorData.name));
			}
		}
	}

	return totalBonus;
}

void Destructible::load(const json& j)
{
	set_max_hp(j.at("hpMax").get<int>());
	set_hp(j.at("hp").get<int>());
	set_hp_base(j.at("hpBase").get<int>());
	set_last_constitution(j.at("lastConstitution").get<int>());
	set_dr(j.at("dr").get<int>());
	set_corpse_name(j.at("corpseName").get<std::string>());
	set_xp(j.at("xp").get<int>());
	set_thaco(j.at("thaco").get<int>());
	set_armor_class(j.at("armorClass").get<int>());
	set_base_armor_class(j.at("baseArmorClass").get<int>());
	set_temp_hp(j.at("tempHp").get<int>());
}

void Destructible::save(json& j)
{
	j["hpMax"] = get_max_hp();
	j["hp"] = get_hp();
	j["hpBase"] = get_hp_base();
	j["lastConstitution"] = get_last_constitution();
	j["dr"] = get_dr();
	j["corpseName"] = get_corpse_name();
	j["xp"] = get_xp();
	j["thaco"] = get_thaco();
	j["armorClass"] = get_armor_class();
	j["baseArmorClass"] = get_base_armor_class();
	j["tempHp"] = get_temp_hp();
}

void PlayerDestructible::save(json& j)
{
	j["type"] = static_cast<int>(DestructibleType::PLAYER);
	Destructible::save(j);
}

void MonsterDestructible::save(json& j)
{
	j["type"] = static_cast<int>(DestructibleType::MONSTER);
	Destructible::save(j);
}

[[nodiscard]] std::unique_ptr<Destructible> Destructible::create(const json& j)
{
	if (j.contains("type") && j["type"].is_number())
	{
		const auto type = static_cast<DestructibleType>(j["type"].get<int>());
		std::unique_ptr<Destructible> destructible{};
		switch (type)
		{

		case DestructibleType::MONSTER:
		{
			destructible = std::make_unique<MonsterDestructible>(0, 0, "", 0, 0, 0);
			break;
		}

		case DestructibleType::PLAYER:
		{
			destructible = std::make_unique<PlayerDestructible>(0, 0, "", 0, 0, 0);
			break;
		}

		default:
		{
			break;
		}
		}

		if (destructible)
		{
			destructible->load(j);
		}

		return destructible;
	}

	return nullptr;
}

//==PlayerDestructible==
PlayerDestructible::PlayerDestructible(
	int hpMax,
	int dr,
	std::string_view corpseName,
	int xp,
	int thaco,
	int armorClass)
	: Destructible(hpMax, dr, corpseName, xp, thaco, armorClass)
{
}

void PlayerDestructible::die(Creature& owner, GameContext& ctx)
{
	ctx.gameState->set_game_status(GameStatus::DEFEAT);

	// Delete save file on death (ignore return value - non-critical operation)
	[[maybe_unused]] const bool deleted = ctx.stateManager->delete_save_file();
}

//==MonsterDestructible==
MonsterDestructible::MonsterDestructible(
	int hpMax,
	int dr,
	std::string_view corpseName,
	int xp,
	int thaco,
	int armorClass)
	: Destructible(hpMax, dr, corpseName, xp, thaco, armorClass)
{
}

void MonsterDestructible::die(Creature& owner, GameContext& ctx)
{
	// message which monster is dead
	ctx.messageSystem->append_message_part(owner.actorData.color, std::format("{}", owner.actorData.name));
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " is dead.\n");
	ctx.messageSystem->finalize_message();

	// get the xp from the monster

	// message how much xp you get
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "You get ");
	ctx.messageSystem->append_message_part(YELLOW_BLACK_PAIR, std::format("{}", get_xp()));
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, " experience points.\n");
	ctx.messageSystem->finalize_message();

	// increase the player's experience
	assert(ctx.player != nullptr && "MonsterDestructible::die requires a live player in context");
	ctx.player->on_kill_reward(get_xp(), ctx);

	if (ctx.animSystem)
	{
		ctx.animSystem->spawn_death(owner.position.x, owner.position.y);
	}

	Destructible::die(owner, ctx);
}

// end of file: Destructible.cpp
