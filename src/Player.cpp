#include <algorithm>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "Actor.h"
#include "PlayerAttacker.h"
#include "EquipmentSlot.h"
#include "ExperienceReward.h"
#include "HealthPool.h"
#include "InventoryOperations.h"
#include "Pickable.h"
#include "Ai.h"
#include "Colors.h"
#include "WeaponDamageRegistry.h"
#include "GameContext.h"
#include "CombatProgressionTables.h"
#include "GameBalance.h"
#include "ItemCreator.h"
#include "ItemClassification.h"
#include "ItemIdentification.h"
#include "Map.h"
#include "Web.h"
#include "Persistent.h"
#include "RandomDice.h"
#include "Renderer.h"
#include "BuffSystem.h"
#include "FloatingTextSystem.h"
#include "BuffType.h"
#include "DisplayManager.h"
#include "GameStateManager.h"
#include "HungerSystem.h"
#include "MessageSystem.h"
#include "RenderingManager.h"
#include "NotificationMenu.h"
#include "SpellSystem.h"
#include "Vector2D.h"
#include "Player.h"

// XP table helpers — pure functions, no state
namespace
{
template <std::size_t N>
constexpr int calculate_xp_for_level(int level, const std::array<int, N>& xpTable, int linearProgression) noexcept
{
	if (level <= 0)
		return xpTable[0];

	if (level < static_cast<int>(xpTable.size()))
		return xpTable[level];

	const int maxLevel = static_cast<int>(xpTable.size()) - 1;
	return xpTable[maxLevel] + (level - maxLevel) * linearProgression;
}

constexpr int calculate_fighter_xp(int level) noexcept
{
	constexpr std::array fighter_xp = {
		0, 2000, 4000, 8000, 16000, 32000, 64000, 125000, 250000, 500000, 750000
	};
	return calculate_xp_for_level(level, fighter_xp, 250000);
}

constexpr int calculate_rogue_xp(int level) noexcept
{
	constexpr std::array rogue_xp = {
		0, 1250, 2500, 5000, 10000, 20000, 40000, 70000, 110000, 160000, 220000
	};
	return calculate_xp_for_level(level, rogue_xp, 60000);
}

constexpr int calculate_cleric_xp(int level) noexcept
{
	constexpr std::array cleric_xp = {
		0, 1500, 3000, 6000, 13000, 27500, 55000, 110000, 225000, 450000, 675000
	};
	return calculate_xp_for_level(level, cleric_xp, 225000);
}

constexpr int calculate_wizard_xp(int level) noexcept
{
	constexpr std::array wizard_xp = {
		0, 2500, 5000, 10000, 20000, 40000, 60000, 90000, 135000, 250000, 375000
	};
	return calculate_xp_for_level(level, wizard_xp, 125000);
}
} // namespace (xp helpers)

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

Player::Player(Vector2D position)
	: Creature(position, ActorData{ TileRef{}, "Player", WHITE_BLACK_PAIR })
{
	set_gold(100);
	controller = std::make_unique<PlayerController>(*this);
}

std::string_view Player::sprite_tile_key() const noexcept
{
	// Only human and dwarf art exists; the other races borrow the human row.
	const bool isDwarf = (playerRaceState == PlayerRaceState::DWARF);

	switch (playerClassState)
	{

	case PlayerClassState::FIGHTER:
	{
		return isDwarf ? "TILE_PLAYER_DWARF_FIGHTER" : "TILE_PLAYER_HUMAN_FIGHTER";
	}

	case PlayerClassState::ROGUE:
	{
		return isDwarf ? "TILE_PLAYER_DWARF_ROGUE" : "TILE_PLAYER_HUMAN_ROGUE";
	}

	case PlayerClassState::CLERIC:
	{
		return isDwarf ? "TILE_PLAYER_DWARF_CLERIC" : "TILE_PLAYER_HUMAN_CLERIC";
	}

	case PlayerClassState::WIZARD:
	{
		return isDwarf ? "TILE_PLAYER_DWARF_WIZARD" : "TILE_PLAYER_HUMAN_WIZARD";
	}

	case PlayerClassState::NONE:
	{
		return "TILE_PLAYER";
	}

	}

	return "TILE_PLAYER";
}

Player::Player(Vector2D position, const PlayerBlueprint& blueprint, GameContext& ctx)
	: Creature(position, ActorData{ TileRef{}, blueprint.name, WHITE_BLACK_PAIR })
{
	set_gender(blueprint.gender);
	playerClass = blueprint.playerClass;
	playerRace = blueprint.playerRace;

	auto applyClassData = [this](std::string_view name)
	{
		if (name == "Fighter")
		{
			playerClassState = PlayerClassState::FIGHTER;
			set_creature_class(CreatureClass::FIGHTER);
			set_hit_die(10);
		}
		else if (name == "Rogue")
		{
			playerClassState = PlayerClassState::ROGUE;
			set_creature_class(CreatureClass::ROGUE);
			set_hit_die(6);
		}
		else if (name == "Cleric")
		{
			playerClassState = PlayerClassState::CLERIC;
			set_creature_class(CreatureClass::CLERIC);
			set_hit_die(8);
		}
		else if (name == "Wizard")
		{
			playerClassState = PlayerClassState::WIZARD;
			set_creature_class(CreatureClass::WIZARD);
			set_hit_die(4);
		}
	};

	auto applyRaceData = [this](std::string_view name)
	{
		if (name == "Human")
		{
			playerRaceState = PlayerRaceState::HUMAN;
		}
		else if (name == "Dwarf")
		{
			playerRaceState = PlayerRaceState::DWARF;
		}
		else if (name == "Elf")
		{
			playerRaceState = PlayerRaceState::ELF;
		}
		else if (name == "Gnome")
		{
			playerRaceState = PlayerRaceState::GNOME;
		}
		else if (name == "Half-Elf")
		{
			playerRaceState = PlayerRaceState::HALFELF;
		}
		else if (name == "Halfling")
		{
			playerRaceState = PlayerRaceState::HALFLING;
		}
	};

	applyClassData(blueprint.playerClass);
	applyRaceData(blueprint.playerRace);

	set_gold(100);
	controller = std::make_unique<PlayerController>(*this);
	roll_new_character(ctx);

	assert(attacker && "Player requires Attacker");
}

void Player::roll_new_character(GameContext& ctx)
{
	// Rolling for stats
	auto roll3d6 = [&ctx]()
	{
		return ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6();
	};

	set_strength(roll3d6());
	set_dexterity(roll3d6());
	set_constitution(roll3d6());
	set_intelligence(roll3d6());
	set_wisdom(roll3d6());
	set_charisma(roll3d6());

	// The owner's cushion plus one roll of the class's own die, set before this runs.
	const int playerHp = GameBalance::Leveling::HitPoints::STARTING_CUSHION + ctx.dice->roll(1, get_hit_die());
	const int playerDr = 1;
	const int playerXp = 0;
	const int playerAC = 10;

	attacker = std::make_unique<PlayerAttacker>(*this);
	experienceReward = std::make_unique<ExperienceReward>(playerXp);
	set_dr(playerDr);
	set_thaco(0);
	armorClass = std::make_unique<ArmorClass>(playerAC);
	healthPool = std::make_unique<HealthPool>(playerHp);
}

void Player::die(GameContext& ctx)
{
	ctx.gameState->set_game_status(GameStatus::DEFEAT);
	[[maybe_unused]] const bool deleted = ctx.stateManager->delete_save_file();
}

void Player::on_new_game_start(GameContext& ctx)
{
	racial_ability_adjustments(ctx);
	// After the racial adjustments, which can take a halfling's 18 away or give it.
	roll_exceptional_strength(ctx);
	equip_class_starting_gear(ctx);
}

void Player::roll_exceptional_strength(GameContext& ctx)
{
	// PHB character creation: a fighter who is not a halfling, with Strength 18.
	const bool isEligible = get_creature_class() == CreatureClass::FIGHTER && playerRaceState != PlayerRaceState::HALFLING && get_strength() == 18;
	if (!isEligible)
	{
		return;
	}
	set_exceptional_strength(ctx.dice->roll(1, 100));
}

void Player::recalculate_combat_stats()
{
	calculate_thaco();
}

void Player::on_kill_reward(int xp, GameContext& ctx)
{
	set_xp(get_xp() + xp);
	++killCount;
	levelup_update(ctx);
}

void Player::equip_class_starting_gear(GameContext& ctx)
{
	switch (playerClassState)
	{

	case PlayerClassState::FIGHTER:
	{
		int startingGold = (ctx.dice->d4() + ctx.dice->d4() + ctx.dice->d4() + ctx.dice->d4() + ctx.dice->d4()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create("plate_mail", position, ctx), EquipmentSlot::BODY, ctx);
		equip_item(ItemCreator::create("long_sword", position, ctx), EquipmentSlot::RIGHT_HAND, ctx);
		equip_item(ItemCreator::create("medium_shield", position, ctx), EquipmentSlot::LEFT_HAND, ctx);
		equip_item(ItemCreator::create("long_bow", position, ctx), EquipmentSlot::MISSILE_WEAPON, ctx);
		[[maybe_unused]] const auto grantFireballResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("scroll_fireball", position, ctx));
		assert(grantFireballResult.has_value());
		[[maybe_unused]] const auto grantIdentifyScrollResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("identify_scroll", position, ctx));
		assert(grantIdentifyScrollResult.has_value());
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Fighter equipped with plate mail, long sword, shield, long bow, fireball scroll. [DEBUG]", true);
		break;
	}

	case PlayerClassState::ROGUE:
	{
		int startingGold = (ctx.dice->d6() + ctx.dice->d6()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create("leather_armor", position, ctx), EquipmentSlot::BODY, ctx);
		equip_item(ItemCreator::create("dagger", position, ctx), EquipmentSlot::RIGHT_HAND, ctx);
		[[maybe_unused]] const auto grantIdentifyScrollResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("identify_scroll", position, ctx));
		assert(grantIdentifyScrollResult.has_value());
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Rogue equipped with leather armor and dagger.", true);
		break;
	}

	case PlayerClassState::CLERIC:
	{
		int startingGold = (ctx.dice->d6() + ctx.dice->d6() + ctx.dice->d6()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create("chain_mail", position, ctx), EquipmentSlot::BODY, ctx);
		equip_item(ItemCreator::create("mace", position, ctx), EquipmentSlot::RIGHT_HAND, ctx);
		equip_item(ItemCreator::create("medium_shield", position, ctx), EquipmentSlot::LEFT_HAND, ctx);
		[[maybe_unused]] const auto grantHealthPotionResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("health_potion", position, ctx));
		assert(grantHealthPotionResult.has_value());
		[[maybe_unused]] const auto grantHoldPersonResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("scroll_hold_person", position, ctx));
		assert(grantHoldPersonResult.has_value());
		[[maybe_unused]] const auto grantIdentifyScrollResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("identify_scroll", position, ctx));
		assert(grantIdentifyScrollResult.has_value());
		SpellSystem::show_memorization_menu(*this, ctx);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Cleric equipped with chain mail, mace, and shield. Spells memorized.", true);
		break;
	}

	case PlayerClassState::WIZARD:
	{
		int startingGold = (ctx.dice->d4() + ctx.dice->d4()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create("staff", position, ctx), EquipmentSlot::RIGHT_HAND, ctx);
		[[maybe_unused]] const auto grantFireballResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("scroll_fireball", position, ctx));
		assert(grantFireballResult.has_value());
		[[maybe_unused]] const auto grantLightningResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("scroll_lightning", position, ctx));
		assert(grantLightningResult.has_value());
		[[maybe_unused]] const auto grantSleepResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("scroll_sleep", position, ctx));
		assert(grantSleepResult.has_value());
		[[maybe_unused]] const auto grantIdentifyScrollResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("identify_scroll", position, ctx));
		assert(grantIdentifyScrollResult.has_value());
		SpellSystem::show_memorization_menu(*this, ctx);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Wizard equipped with staff. Attack scrolls and spells ready.", true);
		break;
	}

	default:
	{
		break;
	}

	}
}
void Player::racial_ability_adjustments(GameContext& ctx)
{
	// Apply stat changes and collect the display lines in a single pass.
	// Human and Half-elf have no stat modifications (AD&D 2e PHB), so they
	// produce no entries and no popup is shown.
	std::vector<std::string> bonusLines;

	switch (playerRaceState)
	{

	case Player::PlayerRaceState::HUMAN:
	case Player::PlayerRaceState::HALFELF:
	{
		break;
	}

	case Player::PlayerRaceState::DWARF:
	{
		adjust_constitution(1);
		adjust_charisma(-1);
		bonusLines.push_back("+1 Constitution");
		bonusLines.push_back("-1 Charisma");
		break;
	}

	case Player::PlayerRaceState::ELF:
	{
		adjust_dexterity(1);
		adjust_constitution(-1);
		bonusLines.push_back("+1 Dexterity");
		bonusLines.push_back("-1 Constitution");
		break;
	}

	case Player::PlayerRaceState::GNOME:
	{
		adjust_intelligence(1);
		adjust_wisdom(-1);
		bonusLines.push_back("+1 Intelligence");
		bonusLines.push_back("-1 Wisdom");
		break;
	}

	case Player::PlayerRaceState::HALFLING:
	{
		adjust_dexterity(1);
		adjust_strength(-1);
		bonusLines.push_back("+1 Dexterity");
		bonusLines.push_back("-1 Strength");
		break;
	}

	default:
	{
		break;
	}

	}

	if (bonusLines.empty())
	{
		return;
	}

	ctx.menus->push_back(std::make_unique<NotificationMenu>(
		std::format("Racial Traits: {}", playerRace),
		std::move(bonusLines),
		ctx));
}

void Player::calculate_thaco()
{
	// Static instance for efficiency - no need to recreate each time
	static constexpr CombatProgressionTables combatTables;

	switch (playerClassState)
	{

	case PlayerClassState::FIGHTER:
	{
		set_thaco(combatTables.get_fighter(get_creature_level()));
		break;
	}

	case PlayerClassState::ROGUE:
	{
		set_thaco(combatTables.get_rogue(get_creature_level()));
		break;
	}

	case PlayerClassState::CLERIC:
	{
		set_thaco(combatTables.get_cleric(get_creature_level()));
		break;
	}

	case PlayerClassState::WIZARD:
	{
		set_thaco(combatTables.get_wizard(get_creature_level()));
		break;
	}

	case PlayerClassState::NONE:
	{
		break;
	}

	default:
	{
		break;
	}

	}
}

int Player::get_open_locks_skill() const noexcept
{
	if (playerClassState != PlayerClassState::ROGUE)
	{
		return 0;
	}

	// AD&D 2e PHB Open Locks table: base 10 at level 1, +5 per level, cap 95.
	constexpr int BASE_OPEN_LOCKS = 10;
	constexpr int PER_LEVEL_BONUS = 5;
	constexpr int MAX_OPEN_LOCKS = 95;

	int level = get_creature_level();
	int baseChance = BASE_OPEN_LOCKS + (level - 1) * PER_LEVEL_BONUS;

	// Dexterity adjustment for Open Locks (PHB 2e thief skill table):
	// Dex 9-10: -10, Dex 11-12: -5, Dex 13-14: 0, Dex 15-16: +5, Dex 17-18: +10
	int dex = get_dexterity();
	int dexBonus = 0;
	if (dex <= 10)
	{
		dexBonus = -10;
	}
	else if (dex <= 12)
	{
		dexBonus = -5;
	}
	else if (dex <= 14)
	{
		dexBonus = 0;
	}
	else if (dex <= 16)
	{
		dexBonus = 5;
	}
	else
	{
		dexBonus = 10;
	}

	return std::clamp(baseChance + dexBonus, 0, MAX_OPEN_LOCKS);
}

void Player::consume_food(int nutrition, GameContext& ctx)
{
	// Decrease hunger by nutrition value
	ctx.hungerSystem->decrease_hunger(ctx, nutrition);
}

void Player::render(const GameContext& ctx) const noexcept
{
	// First, render the player normally
	Creature::render(ctx);
}

bool Player::rest(GameContext& ctx)
{
	// Check if player is already at full health
	if (get_hp() >= get_max_hp())
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You're already at full health.", true);
		return false;
	}

	// Check if enemies are nearby (within a radius of 5 tiles)
	// Exclude shopkeepers and other neutral NPCs
	for (const auto& creature : *ctx.creatures)
	{
		if (creature && !creature->is_dead())
		{
			// Skip the player themselves
			if (creature.get() == this)
			{
				continue;
			}

			// Skip non-hostile creatures (shopkeepers, etc.)
			if (creature->get_attitude() != Attitude::HOSTILE)
			{
				continue;
			}

			// Calculate distance to creature
			int distance = get_tile_distance(creature->position);

			// If hostile enemy is within 5 tiles, can't rest
			if (distance <= 5)
			{
				ctx.messageSystem->message(WHITE_BLACK_PAIR, "You can't rest with enemies nearby!", true);
				return false;
			}
		}
	}

	// Check if player has enough food (hunger isn't too high)
	if (ctx.hungerSystem->get_hunger_state() == HungerState::STARVING ||
		ctx.hungerSystem->get_hunger_state() == HungerState::DYING)
	{
		ctx.messageSystem->message(WHITE_RED_PAIR, "You're too hungry to rest!", true);
		return false;
	}

	// Show resting animation
	animate_resting(ctx);

	// Rest - heal 20% of max HP
	int healAmount = std::max(1, get_max_hp() / 5);
	int amountHealed = heal(healAmount);

	// Capture the hunger state before and after
	HungerState beforeState = ctx.hungerSystem->get_hunger_state();

	// Consume food (increase hunger)
	const int hungerCost = 50;
	ctx.hungerSystem->increase_hunger(ctx, hungerCost);

	HungerState afterState = ctx.hungerSystem->get_hunger_state();

	// Display message with more detail
	ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, "Resting recovers ");
	ctx.messageSystem->append_message_part(WHITE_GREEN_PAIR, std::to_string(amountHealed));
	ctx.messageSystem->append_message_part(WHITE_GREEN_PAIR, " health");

	if (beforeState != afterState)
	{
		// If hunger state changed, mention it
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ", but you've become ");
		ctx.messageSystem->append_message_part(ctx.hungerSystem->get_hunger_color(), ctx.hungerSystem->get_hunger_state_string().c_str());
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ".");
	}
	else
	{
		ctx.messageSystem->append_message_part(WHITE_BLACK_PAIR, ", consuming your food.");
	}

	ctx.messageSystem->finalize_message();

	// Memorize spells for casters
	if (playerClassState == PlayerClassState::CLERIC || playerClassState == PlayerClassState::WIZARD)
	{
		SpellSystem::show_memorization_menu(*this, ctx);
	}

	// Resting takes time
	ctx.gameState->set_game_status(GameStatus::NEW_TURN);
	return true;
}

void Player::animate_resting(GameContext& ctx)
{
	// Spawn one floating 'z'/'Z' above the player each rest turn.
	// The floating text system handles the animated drift and fade.
	// Alternate lowercase/uppercase by hp recovered so far (varies nicely).
	const bool big = (get_max_hp() - get_hp()) < 5;
	const std::string symbol = big ? "Z" : "z";
	ctx.floatingText->spawn_text(
		Vector2D{ position.x, position.y - 1 },   // a row above the sleeper
		symbol,
		200, 230, 200,
		1.2f);
	ctx.renderingManager->render(ctx);
}

bool Player::attempt_hide(GameContext& ctx)
{
	// Only rogues can hide
	if (playerClassState != PlayerClassState::ROGUE)
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Only rogues can hide in shadows.", true);
		return false;
	}

	// Already invisible
	if (is_invisible())
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You are already hidden.", true);
		return false;
	}

	// Check if enemies can see player
	bool observed = false;
	for (const auto& creature : *ctx.creatures)
	{
		if (creature && !creature->is_dead())
		{
			if (ctx.map->is_in_fov(creature->position))
			{
				observed = true;
				break;
			}
		}
	}

	if (observed)
	{
		ctx.messageSystem->message(RED_BLACK_PAIR, "You cannot hide while being observed!", true);
		return false;
	}

	// Success - hide duration based on level
	int hideDuration = 10 + get_creature_level() * 2;
	ctx.buffSystem->add_buff(*this, BuffType::INVISIBILITY, 0, hideDuration, false);
	ctx.messageSystem->message(CYAN_BLACK_PAIR, std::format("You melt into the shadows... (Hidden for {} turns)", hideDuration), true);
	return true;
}

// Clean Weapon Equipment System using Unique IDs
bool Player::toggle_weapon(uint64_t itemUniqueId, EquipmentSlot preferredSlot, GameContext& ctx)
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
		// Find item in inventory and equip it
		Item* itemToEquip = InventoryOperations::find_item_by_id(inventoryData, itemUniqueId);
		if (itemToEquip)
		{
			// Remove item from inventory
			auto result = InventoryOperations::remove_item_by_id(inventoryData, itemUniqueId);
			if (result.has_value())
			{
				auto itemToEquip = std::move(*result);

				// Determine appropriate slot based on item classification
				EquipmentSlot target_slot;

				if (ItemClassificationUtils::is_ranged_weapon(itemToEquip->itemClass))
				{
					target_slot = EquipmentSlot::MISSILE_WEAPON;
				}
				else
				{
					target_slot = preferredSlot;
				}

				return equip_item(std::move(itemToEquip), target_slot, ctx);
			}
		}
	}
	return false;
}

bool Player::toggle_shield(uint64_t itemUniqueId, GameContext& ctx)
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

bool Player::toggle_equipment(uint64_t itemUniqueId, EquipmentSlot slot, GameContext& ctx)
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

bool Player::equip_item(std::unique_ptr<Item> item, EquipmentSlot slot, GameContext& ctx)
{
	if (!item)
	{
		if (ctx.messageSystem->is_debug_mode())
			ctx.messageSystem->log("DEBUG: equip_item failed - null item");
		return false;
	}

	if (!can_equip(*item, slot))
	{
		if (ctx.messageSystem->is_debug_mode())
		{
			ctx.messageSystem->log("DEBUG: can_equip failed for " + item->actorData.name);
			ctx.messageSystem->log("DEBUG: Item class: " + std::to_string(static_cast<int>(item->itemClass)));
			ctx.messageSystem->log("DEBUG: Is armor: " + std::string(item->is_armor() ? "true" : "false"));
			ctx.messageSystem->log("DEBUG: Slot: " + std::to_string(static_cast<int>(slot)));
			ctx.messageSystem->log("DEBUG: equip_item failed - can_equip returned false for " + item->actorData.name + " in slot " + std::to_string(static_cast<int>(slot)));
		}
		// Return item to inventory since we can't equip it
		[[maybe_unused]] const auto pickUpItemResult = InventoryOperations::add_item_to_inventory(inventoryData, std::move(item), *this, *ctx.dataManager);
		assert(pickUpItemResult.has_value());
		return false;
	}

	// A weapon made for a stronger arm cannot be drawn by this one: the first Baldur's
	// Gate's composite bow "Requires: 18 Strength".
	if (!can_draw(*this, *item))
	{
		ctx.messageSystem->message(
			WHITE_BLACK_PAIR,
			std::format("You are not strong enough to draw the {}.", item->actorData.name),
			true);
		[[maybe_unused]] const auto returnedToPack = InventoryOperations::add_item_to_inventory(inventoryData, std::move(item), *this, *ctx.dataManager);
		assert(returnedToPack.has_value());
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
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "You grip the " + item->actorData.name + " with both hands.", true);
		}
	}

	// Add equipped item
	equippedItems.emplace_back(std::move(item), slot);

	// Mark item as equipped
	equippedItems.back().item->add_state(ActorState::IS_EQUIPPED);

	// Log weapon equip
	if (slot == EquipmentSlot::RIGHT_HAND && equippedItems.back().item->is_weapon())
	{
		std::string weaponDamage = WeaponDamageRegistry::get_damage_roll(equippedItems.back().item->itemKey);
		ctx.messageSystem->log("Equipped " + equippedItems.back().item->actorData.name + " - damage: " + weaponDamage);
	}

	// Update armor class if armor or shield was equipped
	if (slot == EquipmentSlot::BODY || slot == EquipmentSlot::LEFT_HAND)
	{
		update_armor_class(ctx);
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(get_armor_class()) + ".", true);
	}

	return true;
}

bool Player::unequip_item(EquipmentSlot slot, GameContext& ctx)
{
	// erase after move requires iterator; find_if + erase is the correct C++23 pattern here
	auto it = std::ranges::find_if(equippedItems, matches_slot(slot));

	if (it != equippedItems.end())
	{
		// AD&D 2e PHB p.230: cursed items are magically bound to the wearer.
		// Only a Remove Curse spell breaks the bond.
		if (it->item->get_enhancement().blessing == BlessingStatus::CURSED)
		{
			ctx.messageSystem->message(
				RED_BLACK_PAIR,
				std::format("The {} is cursed and cannot be removed!", it->item->actorData.name),
				true);
			return false;
		}

		// Out of the slot list before anything reads it again: the pack's weight check reads
		// Strength, which counts every worn item.
		std::unique_ptr<Item> removed = std::move(it->item);
		equippedItems.erase(it);

		// The pack takes it if it can. A full pack, or one too heavy now - taking off a
		// Strength item lowers what can be carried - leaves it at the wearer's feet; with
		// nowhere at all to put it, it stays on. An item that comes off is never lost.
		const bool fitsInPack = !InventoryOperations::is_inventory_full(inventoryData) && InventoryOperations::is_within_weight_limit(inventoryData, *removed, *this, *ctx.dataManager);
		if (!fitsInPack && InventoryOperations::is_inventory_full(*ctx.floorInventory))
		{
			ctx.messageSystem->message(
				WHITE_BLACK_PAIR,
				std::format("There is nowhere to put the {}, so you keep it on.", removed->actorData.name),
				true);
			equippedItems.emplace_back(std::move(removed), slot);
			return false;
		}

		removed->remove_state(ActorState::IS_EQUIPPED);
		if (fitsInPack)
		{
			[[maybe_unused]] const auto packed = InventoryOperations::add_item_to_inventory(inventoryData, std::move(removed), *this, *ctx.dataManager);
			assert(packed.has_value());
		}
		else
		{
			ctx.messageSystem->message(
				WHITE_BLACK_PAIR,
				std::format("You cannot carry the {} as well, and set it down.", removed->actorData.name),
				true);
			removed->position = position;
			[[maybe_unused]] const auto setDown = InventoryOperations::add_item(*ctx.floorInventory, std::move(removed));
			assert(setDown.has_value());
		}

		// Update armor class if armor or shield was unequipped
		if (slot == EquipmentSlot::BODY || slot == EquipmentSlot::LEFT_HAND)
		{
			update_armor_class(ctx);
			ctx.messageSystem->message(WHITE_BLACK_PAIR, "Your armor class is now " + std::to_string(get_armor_class()) + ".", true);
		}

		return true;
	}

	return false;
}

bool Player::is_slot_occupied(EquipmentSlot slot) const noexcept
{
	return get_equipped_item(slot) != nullptr;
}

bool Player::is_dual_wielding() const noexcept
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

std::string Player::get_equipped_weapon_damage_roll() const noexcept
{
	auto rightHandWeapon = get_equipped_item(EquipmentSlot::RIGHT_HAND);
	if (!rightHandWeapon)
	{
		return WeaponDamageRegistry::get_unarmed_damage();
	}

	// Use pure ItemClass system
	if (rightHandWeapon->is_weapon())
	{
		return WeaponDamageRegistry::get_damage_roll(rightHandWeapon->itemKey);
	}

	return WeaponDamageRegistry::get_unarmed_damage();
}

Player::DualWieldInfo Player::get_dual_wield_info() const noexcept
{
	DualWieldInfo info;

	// Check if dual wielding
	if (!is_dual_wielding())
	{
		return info; // Not dual wielding
	}

	info.isDualWielding = true;

	// AD&D 2e Two-Weapon Fighting penalties
	// Base penalties: -2 main hand, -4 off-hand
	info.mainHandPenalty = -2;
	info.offHandPenalty = -4;

	// Fighters have Two-Weapon Fighting proficiency: penalties reduce to 0/-2
	if (playerClassState == PlayerClassState::FIGHTER)
	{
		info.mainHandPenalty = 0;
		info.offHandPenalty = -2;
	}

	// Get off-hand weapon damage roll - use pure ItemClass system
	auto leftHandWeapon = get_equipped_item(EquipmentSlot::LEFT_HAND);
	if (leftHandWeapon)
	{
		if (leftHandWeapon->is_weapon())
		{
			info.offHandDamageRoll = WeaponDamageRegistry::get_damage_roll(leftHandWeapon->itemKey);
		}
	}

	return info;
}

// Clean Equipment System using Unique IDs
bool Player::toggle_armor(uint64_t itemUniqueId, GameContext& ctx)
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

bool Player::is_item_equipped(uint64_t itemUniqueId) const noexcept
{
	return std::ranges::any_of(equippedItems, matches_unique_id(itemUniqueId));
}

void Player::save(json& j)
{
	Creature::save(j); // Call base class save

	// Player-specific fields
	j["playerRaceState"] = static_cast<int>(playerRaceState);
	j["playerClassState"] = static_cast<int>(playerClassState);
	j["playerClass"] = playerClass;
	j["playerRace"] = playerRace;
	j["roundCounter"] = roundCounter;
	j["killCount"] = killCount;
	j["memorizedSpells"] = memorizedSpells;

	// Save equipped items
	json equippedJson = json::array();
	for (const auto& equipped : equippedItems)
	{
		json itemEntry;
		itemEntry["slot"] = static_cast<int>(equipped.slot);
		json itemJson;
		equipped.item->save(itemJson);
		itemEntry["item"] = itemJson;
		equippedJson.push_back(itemEntry);
	}
	j["equippedItems"] = equippedJson;
}

void Player::load(const json& j)
{
	Creature::load(j); // Call base class load
	// Creature::load constructs MonsterAttacker when attacker data is present.
	// Replace with the correct PlayerAttacker strategy.
	attacker = std::make_unique<PlayerAttacker>(*this);
	// PlayerController is not in the Ai hierarchy -- construct directly.
	controller = std::make_unique<PlayerController>(*this);

	// Player-specific fields
	playerRaceState = static_cast<PlayerRaceState>(j.at("playerRaceState").get<int>());
	playerClassState = static_cast<PlayerClassState>(j.at("playerClassState").get<int>());
	playerClass = j.at("playerClass").get<std::string>();
	playerRace = j.at("playerRace").get<std::string>();
	roundCounter = j.at("roundCounter").get<int>();
	killCount = j.at("killCount").get<int>();
	memorizedSpells = j.at("memorizedSpells").get<std::vector<std::string>>();
	// trappingWeb is not serialized - it's a map reference that needs to be re-established

	// Load equipped items
	equippedItems.clear();
	for (const auto& itemEntry : j.at("equippedItems"))
	{
		EquipmentSlot slot = static_cast<EquipmentSlot>(itemEntry.at("slot").get<int>());
		auto item = std::make_unique<Item>(Vector2D{ 0, 0 }, ActorData{ TileRef{}, "temp", 0 });
		item->load(itemEntry.at("item"));
		equippedItems.emplace_back(std::move(item), slot);
	}
}

void Player::update(GameContext& ctx)
{
	update_creature_state(ctx);
	assert(controller && "Player::update called with null controller");
	controller->update(ctx);
}

void Player::apply_confusion(int duration)
{
	assert(controller && "Player::apply_confusion called with null controller");
	controller->apply_confusion(duration);
}

int Player::get_next_level_xp() const
{
	int currentLevel = get_creature_level();

	switch (playerClassState)
	{

	case PlayerClassState::FIGHTER:
	{
		return calculate_fighter_xp(currentLevel);
	}

	case PlayerClassState::ROGUE:
	{
		return calculate_rogue_xp(currentLevel);
	}

	case PlayerClassState::CLERIC:
	{
		return calculate_cleric_xp(currentLevel);
	}

	case PlayerClassState::WIZARD:
	{
		return calculate_wizard_xp(currentLevel);
	}

	default:
	{
		return 2000 * currentLevel;
	}

	}
}

void Player::levelup_update(GameContext& ctx)
{
	ctx.messageSystem->log("Player::levelup_update");
	int levelUpXp = get_next_level_xp();
	if (get_xp() >= levelUpXp)
	{
		adjust_level(1);
		set_xp(get_xp() - levelUpXp);
		ctx.messageSystem->message(
			WHITE_BLACK_PAIR,
			std::format("Your battle skills grow stronger! You reached level {}", get_creature_level()),
			true);

		if (ctx.displayManager != nullptr)
		{
			ctx.displayManager->display_levelup(*this, get_creature_level(), ctx);
		}
	}
}
