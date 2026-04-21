#pragma once

class Item;
class Player;
struct GameContext;

// CurseSystem handles per-turn curse notifications and the amulet HP drain.
//
// Mechanical penalties live at their computation sites and are NOT duplicated here:
//   Weapon  -2 to hit  : PlayerAttacker::attack (curse_hit_penalty lambda)
//   Armor   +1 AC      : Destructible::calculate_equipment_ac_bonus
//   Ring    stat penalty: ItemEnhancement strength/dexterity bonuses
//                         applied by Player::add_stat_bonuses_from_equipment on equip
//   Lock-in (all types): Player::unequip_item guards on BlessingStatus::CURSED
//
// Per-turn notification messages ARE emitted here so the player sees feedback
// each turn a cursed item is worn.
class CurseSystem
{
public:
    CurseSystem() = default;
    ~CurseSystem() = default;

    // Called once per NEW_TURN from GameLoopCoordinator::update().
    void apply_curses(Player& player, GameContext& ctx);

private:
    // Emits "weakens your aim" notification for cursed weapons.
    void apply_weapon_curse(const Item& item, GameContext& ctx);

    // Emits "deteriorates" notification for cursed armor.
    void apply_armor_curse(const Item& item, GameContext& ctx);

    // Drains 1 HP per turn and emits "drains" notification for cursed amulets.
    void apply_hp_drain(int damage, Player& player, GameContext& ctx);
};
