#pragma once

class Player;
struct GameContext;

// CurseSystem handles the one per-turn curse effect: amulet HP drain.
//
// All other curse mechanics live at their computation sites:
//   Weapon  -2 to hit  : PlayerAttacker::attack (curse_hit_penalty lambda)
//   Armor   +1 AC      : Destructible::calculate_equipment_ac_bonus
//   Ring    stat penalty: ItemEnhancement::strength_bonus / dexterity_bonus
//                         applied by Player::add_stat_bonuses_from_equipment on equip
//   Lock-in (all types): Player::unequip_item guards on BlessingStatus::CURSED
class CurseSystem
{
public:
	CurseSystem() = default;
	~CurseSystem() = default;

	// Called once per NEW_TURN from GameLoopCoordinator::update().
	void apply_curses(Player& player, GameContext& ctx);

private:
	// Drains 1 HP per turn for every cursed amulet the player wears.
	void apply_hp_drain(int damage, Player& player, GameContext& ctx);
};
