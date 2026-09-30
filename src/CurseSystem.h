#pragma once

class Item;
class Player;
struct GameContext;

// CurseSystem handles per-turn curse notifications and the amulet HP drain.
//
// Mechanical penalties live at their computation sites and are NOT duplicated here:
//   Weapon  -2 to hit  : PlayerAttacker::attack (curse_hit_penalty lambda)
//   Armor   +1 AC      : ArmorClass::update (called during creature update)
//   Ring    stat penalty: ItemEnhancement strength/dexterity bonuses, counted
//                         while worn by Creature::calculate_effective_stat
//   Lock-in (all types): Player::unequip_item guards on BlessingStatus::CURSED
//
// Per-turn notification messages ARE emitted here so the player sees feedback
// each turn a cursed item is worn.
class CurseSystem
{
private:
	void apply_weapon_curse(const Item& item, GameContext& ctx);
	void apply_armor_curse(const Item& item, GameContext& ctx);

public:
	void apply_curses(Player& player, GameContext& ctx);

	// Takes `damage` from the wearer, says so, and ends the game if it was the
	// last of their health. Public because the amount is the whole of what it
	// does: a cursed amulet costs one a turn today, and the drain that kills is a
	// different case from the drain that does not.
	void apply_hp_drain(int damage, Player& player, GameContext& ctx);
};
