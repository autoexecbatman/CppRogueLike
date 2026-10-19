// file: Player.cpp
//
// The character the person is playing: what only the player has, and what the
// player does differently from every other creature.
//
// Player derives from Creature and overrides the handful of behaviours where
// being the player changes the answer - dying ends the run rather than dropping
// loot, and loading rebuilds the two components a creature's record never
// carries. Everything a monster also has lives on Creature; reach it through
// ctx.player() for those, and ctx.player_concrete() for what is here.
//
// Usage:
//
//   auto player = std::make_unique<Player>(Vector2D{ 1, 1 });   // an empty shell
//   player->on_new_game_start(ctx);                             // rolls race, class, scores
//
//   json record;
//   player->save(record);                                       // the whole character
//   player->load(record);                                       // and back, components included
//
//   player->die(ctx);                                           // the run is lost; the loop does the rest
//
// The one thing to know before editing load(): Creature::load builds the health
// pool, the armour class and the experience reward only when the record carries
// them, which is right for a monster and wrong here. The assertion closing
// Player::load is what keeps that from loading quietly.

#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "Actor.h"
#include "BuffSystem.h"
#include "BuffType.h"
#include "Colors.h"
#include "CombatProgressionTables.h"
#include "DisplayManager.h"
#include "EquipmentSlot.h"
#include "ExperienceReward.h"
#include "FloatingTextSystem.h"
#include "GameBalance.h"
#include "GameContext.h"
#include "HealthPool.h"
#include "HungerSystem.h"
#include "InventoryOperations.h"
#include "ItemCreator.h"
#include "MenuThiefSkills.h"
#include "MessageSystem.h"
#include "NotificationMenu.h"
#include "Persistent.h"
#include "Player.h"
#include "PlayerAttacker.h"
#include "PlayerTurn.h"
#include "RandomDice.h"
#include "Renderer.h"
#include "RenderingManager.h"
#include "SpellSystem.h"
#include "Vector2D.h"
#include "WeaponDamageRegistry.h"

// XP table helpers - pure functions, no state
namespace
{
template <std::size_t N>
constexpr int calculate_xp_for_level(int level, const std::array<int, N>& xpTable, int linearProgression) noexcept
{
	if (level <= 0)
	{
		return xpTable[0];
	}

	if (level < static_cast<int>(xpTable.size()))
	{
		return xpTable[level];
	}

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
} // namespace

Player::Player(Vector2D position)
	: Creature(position, ActorData{ TileRef{}, "Player", ColorPairId::WHITE_BLACK })
{
	set_gold(100);
	controller = std::make_unique<PlayerController>(*this);
}

std::string_view Player::sprite_tile_key() const noexcept
{
	// Only human and dwarf art exists; the other races borrow the human row.
	const bool isDwarf = (playerRaceState == PlayerRaceState::DWARF);

	switch (get_creature_class())
	{

	case CreatureClass::FIGHTER:
	{
		return isDwarf ? "TILE_PLAYER_DWARF_FIGHTER" : "TILE_PLAYER_HUMAN_FIGHTER";
	}

	case CreatureClass::ROGUE:
	{
		return isDwarf ? "TILE_PLAYER_DWARF_ROGUE" : "TILE_PLAYER_HUMAN_ROGUE";
	}

	case CreatureClass::CLERIC:
	{
		return isDwarf ? "TILE_PLAYER_DWARF_CLERIC" : "TILE_PLAYER_HUMAN_CLERIC";
	}

	case CreatureClass::WIZARD:
	{
		return isDwarf ? "TILE_PLAYER_DWARF_WIZARD" : "TILE_PLAYER_HUMAN_WIZARD";
	}

	case CreatureClass::MONSTER:
	{
		assert(false && "Player::sprite_tile_key: a player with a monster's class");
		return "TILE_PLAYER";
	}
	}

	std::unreachable();
}

Player::Player(Vector2D position, const PlayerBlueprint& blueprint, GameContext& ctx)
	: Creature(position, ActorData{ TileRef{}, blueprint.name, ColorPairId::WHITE_BLACK })
{
	set_gender(blueprint.gender);
	playerRaceState = blueprint.playerRaceState;

	// The class is the blueprint's enum, and its hit die is the class's: a warrior's d10,
	// a priest's d8, a rogue's d6, a wizard's d4.
	set_creature_class(blueprint.creatureClass);
	switch (blueprint.creatureClass)
	{
	case CreatureClass::FIGHTER:
	{
		set_hit_die(10);
		break;
	}
	case CreatureClass::ROGUE:
	{
		set_hit_die(6);
		break;
	}
	case CreatureClass::CLERIC:
	{
		set_hit_die(8);
		break;
	}
	case CreatureClass::WIZARD:
	{
		set_hit_die(4);
		break;
	}
	case CreatureClass::MONSTER:
	{
		assert(false && "Player: a blueprint reached construction with no class chosen");
		break;
	}
	}

	// The six come from the allocation screen rather than from dice here: Method VI
	// is what character creation runs, and the blueprint carries what it produced.
	for (const Ability ability : ALL_ABILITY)
	{
		set_ability(ability, blueprint.abilityScores.at(ability_index(ability)));
	}

	set_gold(100);
	controller = std::make_unique<PlayerController>(*this);
	roll_new_character(ctx);

	assert(attacker && "Player requires Attacker");
}

std::string Player::get_race_display_name() const
{
	return std::string{ race_display_name(playerRaceState) };
}

void Player::roll_new_character(GameContext& ctx)
{
	assert(ctx.dice && "Player::roll_new_character: called without dice");

	// The six are allocated before the character exists, so nothing here touches them.
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
	// Only declares the run lost; the loop removes the save, so a death never reaches the filesystem.
	ctx.gameState->set_run_outcome(RunOutcome::DEFEAT);
}

void Player::on_new_game_start(GameContext& ctx)
{
	std::vector<std::string> racialLines = pay_racial_adjustments();
	// After the racial adjustments, since the roll reads the Strength they leave.
	roll_exceptional_strength(ctx);
	equip_class_starting_gear(ctx);
	// After the gear: Table 29 reads the armour the kit puts on a rogue, so an earlier screen shows wrong numbers.
	push_thief_skill_allocation(*this, get_creature_level(), ctx);
	// Pushed last because the menu stack runs its newest entry: the popup explaining the
	// race is read before the skill screen that is built on the scores it changed.
	show_racial_adjustments(std::move(racialLines), ctx);
}

void Player::roll_exceptional_strength(GameContext& ctx)
{
	assert(ctx.dice && "Player::roll_exceptional_strength: called without dice");

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
	assert(ctx.dice && "Player::equip_class_starting_gear: called without dice");

	switch (get_creature_class())
	{

	case CreatureClass::FIGHTER:
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
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Fighter equipped with plate mail, long sword, shield, long bow, fireball scroll. [DEBUG]", MessageCompletion::FINISHED);
		break;
	}

	case CreatureClass::ROGUE:
	{
		int startingGold = (ctx.dice->d6() + ctx.dice->d6()) * 10;
		set_gold(startingGold);
		equip_item(ItemCreator::create("leather_armor", position, ctx), EquipmentSlot::BODY, ctx);
		equip_item(ItemCreator::create("dagger", position, ctx), EquipmentSlot::RIGHT_HAND, ctx);
		[[maybe_unused]] const auto grantIdentifyScrollResult = InventoryOperations::add_item(inventoryData, ItemCreator::create("identify_scroll", position, ctx));
		assert(grantIdentifyScrollResult.has_value());
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Rogue equipped with leather armor and dagger.", MessageCompletion::FINISHED);
		break;
	}

	case CreatureClass::CLERIC:
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
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Cleric equipped with chain mail, mace, and shield. Spells memorized.", MessageCompletion::FINISHED);
		break;
	}

	case CreatureClass::WIZARD:
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
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "Wizard equipped with staff. Attack scrolls and spells ready.", MessageCompletion::FINISHED);
		break;
	}

	case CreatureClass::MONSTER:
	{
		assert(false && "Player::equip_class_starting_gear: a player with a monster's class");
		break;
	}
	}
}

std::string_view class_display_name(CreatureClass creatureClass)
{
	switch (creatureClass)
	{
	case CreatureClass::FIGHTER:
	{
		return "Fighter";
	}
	case CreatureClass::ROGUE:
	{
		return "Rogue";
	}
	case CreatureClass::CLERIC:
	{
		return "Cleric";
	}
	case CreatureClass::WIZARD:
	{
		return "Wizard";
	}
	case CreatureClass::MONSTER:
	{
		assert(false && "class_display_name: a monster's class has no player-facing name");
		return "Monster";
	}
	}

	std::unreachable();
}

std::string Player::get_class_display_name() const
{
	return std::string{ class_display_name(get_creature_class()) };
}

std::string_view race_display_name(PlayerRaceState race)
{
	switch (race)
	{
	case PlayerRaceState::HUMAN:
	{
		return "Human";
	}
	case PlayerRaceState::ELF:
	{
		return "Elf";
	}
	case PlayerRaceState::DWARF:
	{
		return "Dwarf";
	}
	case PlayerRaceState::HALFLING:
	{
		return "Halfling";
	}
	case PlayerRaceState::GNOME:
	{
		return "Gnome";
	}
	case PlayerRaceState::HALFELF:
	{
		return "Half-Elf";
	}
	case PlayerRaceState::NONE:
	{
		return "None";
	}
	}

	return "None";
}

std::array<int, ABILITY_COUNT> racial_ability_modifiers(PlayerRaceState race)
{
	switch (race)
	{
	case PlayerRaceState::DWARF:
	{
		return { 0, 0, 1, 0, 0, -1 };
	}
	case PlayerRaceState::ELF:
	{
		return { 0, 1, -1, 0, 0, 0 };
	}
	case PlayerRaceState::GNOME:
	{
		return { 0, 0, 0, 1, -1, 0 };
	}
	case PlayerRaceState::HALFLING:
	{
		return { -1, 1, 0, 0, 0, 0 };
	}
	case PlayerRaceState::HUMAN:
	case PlayerRaceState::HALFELF:
	case PlayerRaceState::NONE:
	{
		return {};
	}
	}

	return {};
}

std::vector<std::string> Player::pay_racial_adjustments()
{
	// Pay the race and collect the display lines in a single pass. Human and
	// half-elf modify nothing, so they produce no entries and no popup is shown.
	const std::array<int, ABILITY_COUNT> modifiers = racial_ability_modifiers(playerRaceState);
	std::vector<std::string> bonusLines;

	for (const Ability ability : ALL_ABILITY)
	{
		const int modifier = modifiers.at(ability_index(ability));
		if (modifier == 0)
		{
			continue;
		}

		adjust_ability(ability, modifier);
		bonusLines.push_back(std::format("{:+} {}", modifier, ability_name(ability)));
	}

	return bonusLines;
}

void Player::show_racial_adjustments(std::vector<std::string> bonusLines, GameContext& ctx)
{
	if (bonusLines.empty())
	{
		return;
	}

	ctx.menus->push_back(std::make_unique<NotificationMenu>(
		std::format("Racial Traits: {}", race_display_name(playerRaceState)),
		std::move(bonusLines),
		ctx));
}

void Player::calculate_thaco()
{
	// Static instance for efficiency - no need to recreate each time
	static constexpr CombatProgressionTables combatTables;

	switch (get_creature_class())
	{

	case CreatureClass::FIGHTER:
	{
		set_thaco(combatTables.get_fighter(get_creature_level()));
		break;
	}

	case CreatureClass::ROGUE:
	{
		set_thaco(combatTables.get_rogue(get_creature_level()));
		break;
	}

	case CreatureClass::CLERIC:
	{
		set_thaco(combatTables.get_cleric(get_creature_level()));
		break;
	}

	case CreatureClass::WIZARD:
	{
		set_thaco(combatTables.get_wizard(get_creature_level()));
		break;
	}

	case CreatureClass::MONSTER:
	{
		assert(false && "Player::calculate_thaco: a player with a monster's class");
		break;
	}

	default:
	{
		break;
	}
	}
}

std::array<int, THIEF_SKILL_COUNT> thief_skill_racial_adjustments(PlayerRaceState race)
{
	// Table 27 across each race's column: Pick Pockets, Open Locks, Find/Remove Traps, Move Silently,
	// Hide in Shadows, Detect Noise, Climb Walls, Read Languages.
	switch (race)
	{
	case PlayerRaceState::DWARF:
	{
		return { 0, 10, 15, 0, 0, 0, -10, -5 };
	}
	case PlayerRaceState::ELF:
	{
		return { 5, -5, 0, 5, 10, 5, 0, 0 };
	}
	case PlayerRaceState::GNOME:
	{
		return { 0, 5, 10, 5, 5, 10, -15, 0 };
	}
	case PlayerRaceState::HALFELF:
	{
		return { 10, 0, 0, 0, 5, 0, 0, 0 };
	}
	case PlayerRaceState::HALFLING:
	{
		return { 5, 5, 5, 10, 15, 5, -15, -5 };
	}
	case PlayerRaceState::HUMAN:
	case PlayerRaceState::NONE:
	{
		return {};
	}
	}

	return {};
}

std::optional<ThiefArmor> Player::thief_armor() const noexcept
{
	// Nothing on the body is Table 29's "No Armor" column, which is a bonus.
	const Item* worn = get_equipped_item(EquipmentSlot::BODY);
	if (worn == nullptr)
	{
		return ThiefArmor::NONE;
	}

	return thief_armor_column(worn->itemKey);
}

std::optional<int> Player::thief_skill(ThiefSkill skill) const noexcept
{
	// The thief skills belong to the one class the book gives them to.
	if (get_creature_class() != CreatureClass::ROGUE)
	{
		return std::nullopt;
	}

	// Armour Table 29 has no column for leaves the skill with no number at all.
	const std::optional<ThiefArmor> armor = thief_armor();
	if (!armor.has_value())
	{
		return std::nullopt;
	}

	return thief_skill_score(
		skill,
		thief_skill_racial_adjustments(playerRaceState).at(thief_skill_index(skill)),
		get_dexterity(),
		armor.value(),
		thiefSkillPoints.at(thief_skill_index(skill)));
}

void Player::consume_food(int nutrition, GameContext& ctx)
{
	assert(ctx.hungerSystem && "Player::consume_food: called without a hungerSystem");

	// Decrease hunger by nutrition value
	ctx.hungerSystem->decrease_hunger(ctx, nutrition);
}

void Player::render(const GameContext& ctx) const noexcept
{
	// First, render the player normally
	Creature::render(ctx);
}

// Heals the player a fifth of their maximum, at the cost of a turn's worth of hunger,
// and reports whether any of that happened.
//
// It refuses three ways, each with a message: at full health there is nothing to
// heal; with a living hostile creature within REST_DANGER_RADIUS tiles it is not safe
// to stop; and starving or dying, the body has nothing to mend with. Only a hostile
// blocks it, which is what lets a player rest in a shop with the shopkeeper beside
// them. The healing is floored at one point, so a character whose maximum is under
// five still gains something.
//
// Example (the refusals are what PlayerRestTest runs):
//   player.rest(ctx);  // at full health
//                      // -> false, "You're already at full health."
//   player.rest(ctx);  // hurt, with a living orc five tiles away
//                      // -> false, "You can't rest with enemies nearby!"
//   player.rest(ctx);  // hurt, nothing hostile nearby
//                      // -> true, healed by a fifth of the maximum
bool Player::rest(GameContext& ctx)
{
	assert(ctx.hungerSystem && "Player::rest: called without a hungerSystem");
	assert(ctx.creatures && "Player::rest: called without creatures");

	// Check if player is already at full health
	if (get_hp() >= get_max_hp())
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You're already at full health.", MessageCompletion::FINISHED);
		return false;
	}

	// Anything hostile and alive this close stops the rest. Shopkeepers and other
	// neutral creatures do not, which is what lets a player rest inside a shop.
	for (const auto& creature : *ctx.creatures)
	{
		assert(creature && "Player::rest: creatures list holds a null entry");
		// The player is reached through ctx.player(), never through this list.
		assert(creature.get() != this && "Player::rest: the creatures list holds the player");

		if (creature->is_dead())
		{
			continue;
		}

		// Skip non-hostile creatures (shopkeepers, etc.)
		if (creature->get_attitude() != Attitude::HOSTILE)
		{
			continue;
		}

		// Chebyshev, so the danger zone is a square: a creature on the diagonal is
		// as near as one straight ahead.
		if (get_tile_distance(creature->position) <= REST_DANGER_RADIUS)
		{
			ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You can't rest with enemies nearby!", MessageCompletion::FINISHED);
			return false;
		}
	}

	// Check if player has enough food (hunger isn't too high)
	if (ctx.hungerSystem->get_hunger_state() == HungerState::STARVING ||
		ctx.hungerSystem->get_hunger_state() == HungerState::DYING)
	{
		ctx.messageSystem->message(ColorPairId::WHITE_RED, "You're too hungry to rest!", MessageCompletion::FINISHED);
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
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, "Resting recovers ");
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_GREEN, std::to_string(amountHealed));
	ctx.messageSystem->append_message_part(ColorPairId::WHITE_GREEN, " health");

	if (beforeState != afterState)
	{
		// If hunger state changed, mention it
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, ", but you've become ");
		ctx.messageSystem->append_message_part(ctx.hungerSystem->get_hunger_color(), ctx.hungerSystem->get_hunger_state_string().c_str());
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, ".");
	}
	else
	{
		ctx.messageSystem->append_message_part(ColorPairId::WHITE_BLACK, ", consuming your food.");
	}

	ctx.messageSystem->finalize_message();

	// Memorize spells for casters
	if (get_creature_class() == CreatureClass::CLERIC || get_creature_class() == CreatureClass::WIZARD)
	{
		SpellSystem::show_memorization_menu(*this, ctx);
	}

	// Resting takes time
	spend_player_action(ctx, TIME_UNITS_PER_ROUND);
	return true;
}

void Player::animate_resting(GameContext& ctx)
{
	assert(ctx.renderingManager && "Player::animate_resting: called without a renderingManager");

	// One floating z above the player each rest turn, a capital Z within 5 hit points of full.
	const bool big = (get_max_hp() - get_hp()) < 5;
	const std::string symbol = big ? "Z" : "z";
	assert(ctx.floatingText && "animate_resting: a rest was shown with no floating text system");
	ctx.floatingText->spawn_text(
		Vector2D{ position.x, position.y - 1 }, // a row above the sleeper
		symbol,
		200,
		230,
		200,
		1.2f);
	ctx.renderingManager->render(ctx);
}

Player::HideAttempt Player::attempt_hide(GameContext& ctx)
{
	assert(ctx.creatures && "Player::attempt_hide: called without creatures");
	assert(ctx.dice && "Player::attempt_hide: called without dice");

	// Hide in Shadows is the thief's, and a score the adjustments leave at nothing
	// is one the book says has to be bought up before it can be used at all.
	const std::optional<int> skill = thief_skill(ThiefSkill::HIDE_IN_SHADOWS);
	if (!skill.has_value() || skill.value() <= 0)
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You cannot hide in shadows.", MessageCompletion::FINISHED);
		return HideAttempt::NOT_A_THIEF;
	}

	// Nothing to try: the thief believes he is in the shadows whatever the roll gave, or a
	// spell already hides him.
	if (is_in_shadows() || is_invisible())
	{
		ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You are already hidden.", MessageCompletion::FINISHED);
		return HideAttempt::ALREADY_HIDDEN;
	}

	// "A thief can never become hidden while a guard is watching him": watching is awareness,
	// whether anything is looking for him, not whether he can see it.
	const int currentTime = ctx.gameState->get_time();
	const auto is_watching = [currentTime](const std::unique_ptr<Creature>& creature)
	{
		assert(creature && "attempt_hide: creatures list holds a null entry");
		return !creature->is_dead() && creature->is_aware(currentTime);
	};
	if (std::ranges::any_of(*ctx.creatures, is_watching))
	{
		ctx.messageSystem->message(ColorPairId::RED_BLACK, "You cannot hide while watched.", MessageCompletion::FINISHED);
		return HideAttempt::WATCHED;
	}

	// The thief always thinks he is hidden, so the spot and the line are his whatever the
	// secret roll gave. The book gives a hide no duration; moving or attacking ends it.
	hidingSpot = position;
	if (ctx.dice->d100() <= skill.value())
	{
		ctx.buffSystem->add_buff_until_broken(*this, BuffType::HIDDEN_IN_SHADOWS);
	}
	ctx.messageSystem->message(ColorPairId::CYAN_BLACK, "You melt into the shadows...", MessageCompletion::FINISHED);
	return HideAttempt::ATTEMPTED;
}

bool Player::is_in_shadows() const noexcept
{
	return hidingSpot.has_value();
}

void Player::leave_shadows(GameContext& ctx)
{
	assert(hidingSpot.has_value() && "Player::leave_shadows: called on a thief who is not in the shadows");
	assert(ctx.buffSystem && "Player::leave_shadows: called without a buffSystem");
	assert(ctx.messageSystem && "Player::leave_shadows: called without a messageSystem");

	// The buff is there only if the roll made it; the line is printed either way.
	ctx.buffSystem->remove_buff(*this, BuffType::HIDDEN_IN_SHADOWS);
	hidingSpot.reset();
	ctx.messageSystem->message(ColorPairId::WHITE_BLACK, "You leave the shadows.", MessageCompletion::FINISHED);
}

void Player::leave_shadows_if_moved(GameContext& ctx)
{
	assert(ctx.buffSystem && "Player::leave_shadows_if_moved: called without a buffSystem");

	// The invariant: a hide the roll made was taken from a spot, so the buff never outlives it.
	assert((!ctx.buffSystem->has_buff(*this, BuffType::HIDDEN_IN_SHADOWS) || hidingSpot.has_value()) && "Player::leave_shadows_if_moved: a hide is live with no spot it was taken from");

	if (hidingSpot.has_value() && *hidingSpot != position)
	{
		leave_shadows(ctx);
	}
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
	if (get_creature_class() == CreatureClass::FIGHTER)
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

// Writes the whole character into j: the creature half through Creature::save, which
// includes every equipped item with the slot it sits in, then the fields only a player has.
// Every field written here is required on load - there are no save fallbacks.
//
// Example:
//
//   json record;
//   player->save(record);
//   record["killCount"];              // -> 7
//   record["equippedItems"].size();   // -> 2
void Player::save(json& j)
{
	Creature::save(j); // Call base class save

	// The controller owns the confusion timer, so it writes its own section rather
	// than the player reaching in for one field.
	assert(controller && "Player::save called with null controller");
	json controllerJson;
	controller->save(controllerJson);
	j["controller"] = controllerJson;

	// Player-specific fields
	j["playerRaceState"] = static_cast<int>(playerRaceState);
	j["killCount"] = killCount;
	j["memorizedSpells"] = memorizedSpells;
	j["thiefSkillPoints"] = thiefSkillPoints;
	j["hidingSpot"] = hidingSpot.has_value()
		? json{ { "x", hidingSpot->x }, { "y", hidingSpot->y } }
		: json(nullptr);
}

// Rebuilds the character from j, and refuses a record that is missing anything.
//
// Two components are always replaced rather than read: Creature::load builds a
// MonsterAttacker from the attacker record, which is wrong for a player, and
// PlayerController is not in the Ai hierarchy so nothing else would construct it.
// Three more - the health pool, the armour class and the experience reward - are
// built by Creature::load only when the record carries them, so the assertion at the
// end is what stops a truncated record producing a player the game cannot run.
//
// Throws nlohmann::json::out_of_range if any required field is absent.
//
// Example:
//
//   json record;
//   saved->save(record);
//   loaded->load(record);
//   loaded->get_max_hp();     // -> 30, the value that was saved
//
//   record.erase("killCount");
//   loaded->load(record);     // throws: no field this saver writes is optional
void Player::load(const json& j)
{
	Creature::load(j); // Call base class load
	// Creature::load constructs MonsterAttacker when attacker data is present.
	// Replace with the correct PlayerAttacker strategy.
	attacker = std::make_unique<PlayerAttacker>(*this);
	// PlayerController is not in the Ai hierarchy -- construct directly.
	controller = std::make_unique<PlayerController>(*this);

	// After construction, so the fresh controller cannot overwrite the record; a save without it is refused.
	controller->load(j.at("controller"));

	// Player-specific fields
	playerRaceState = static_cast<PlayerRaceState>(j.at("playerRaceState").get<int>());
	killCount = j.at("killCount").get<int>();
	memorizedSpells = j.at("memorizedSpells").get<std::vector<std::string>>();
	thiefSkillPoints = j.at("thiefSkillPoints").get<std::array<int, THIEF_SKILL_COUNT>>();
	const json& spot = j.at("hidingSpot");
	hidingSpot = spot.is_null()
		? std::nullopt
		: std::optional<Vector2D>{ Vector2D{ spot.at("x").get<int>(), spot.at("y").get<int>() } };

	// Creature::load builds these only when the record has them, and the HUD reads them all on the first frame.
	assert(healthPool && armorClass && experienceReward && attacker && controller && "Player::load finished with a player the game cannot run");
}

void Player::update(GameContext& ctx)
{
	update_creature_state(ctx);
	assert(controller && "Player::update called with null controller");
	controller->update(ctx);
}

void Player::apply_confusion(int durationRounds, int currentTime)
{
	assert(controller && "Player::apply_confusion called with null controller");
	controller->apply_confusion(durationRounds, currentTime);
}

int Player::get_next_level_xp() const
{
	int currentLevel = get_creature_level();

	switch (get_creature_class())
	{

	case CreatureClass::FIGHTER:
	{
		return calculate_fighter_xp(currentLevel);
	}

	case CreatureClass::ROGUE:
	{
		return calculate_rogue_xp(currentLevel);
	}

	case CreatureClass::CLERIC:
	{
		return calculate_cleric_xp(currentLevel);
	}

	case CreatureClass::WIZARD:
	{
		return calculate_wizard_xp(currentLevel);
	}

	case CreatureClass::MONSTER:
	{
		assert(false && "Player::get_next_level_xp: a player with a monster's class");
		return 2000 * currentLevel;
	}
	}

	std::unreachable();
}

void Player::levelup_update(GameContext& ctx)
{
	assert(ctx.displayManager && "Player::levelup_update: called without a displayManager");

	ctx.messageSystem->log("Player::levelup_update");
	int levelUpXp = get_next_level_xp();
	if (get_xp() >= levelUpXp)
	{
		adjust_level(1);
		set_xp(get_xp() - levelUpXp);
		ctx.messageSystem->message(
			ColorPairId::WHITE_BLACK,
			std::format("Your battle skills grow stronger! You reached level {}", get_creature_level()),
			MessageCompletion::FINISHED);

		ctx.displayManager->display_levelup(*this, get_creature_level(), ctx);
	}
}
