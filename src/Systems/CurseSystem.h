#pragma once

class Item;
class Player;
struct GameContext;

// CurseSystem handles per-turn curse notifications and the amulet HP drain.
//
// Mechanical penalties live at their computation sites and are NOT duplicated here:
//   Weapon  -2 to hit  : PlayerAttacker::attack (curse_hit_penalty lambda)
//   Armor   +1 AC      : ArmorClass::update (called during creature update)
//   Ring    stat penalty: ItemEnhancement strength/dexterity bonuses
//                         applied by Player::add_stat_bonuses_from_equipment on equip
//   Lock-in (all types): Player::unequip_item guards on BlessingStatus::CURSED
//
// Per-turn notification messages ARE emitted here so the player sees feedback
// each turn a cursed item is worn.
class CurseSystem
{
private:
	void apply_weapon_curse(const Item& item, GameContext& ctx);
	void apply_armor_curse(const Item& item, GameContext& ctx);
	void apply_hp_drain(int damage, Player& player, GameContext& ctx);

public:
    void apply_curses(Player& player, GameContext& ctx);

};
