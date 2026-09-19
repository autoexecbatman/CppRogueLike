#pragma once

#include <memory>

class Creature;
struct GameContext;
enum class DamageType;

class HealthPool
{
private:
	int hpBase{};
	int hpMax{};
	int hp{};
	int tempHp{};
	// Fire and acid hit points taken and not yet healed. Constitution regeneration
	// cannot heal them (Player's Handbook, PDF page 33); every other healing can.
	int unregenerableDamage{ 0 };

public:
	HealthPool(int hpMax);
	virtual ~HealthPool() = default;
	HealthPool(const HealthPool&) = delete;
	HealthPool(HealthPool&&) = delete;
	HealthPool& operator=(const HealthPool&) = delete;
	HealthPool& operator=(HealthPool&&) = delete;

	// Query methods
	[[nodiscard]] int get_hp() const noexcept { return hp; }
	[[nodiscard]] int get_max_hp() const noexcept { return hpMax; }
	[[nodiscard]] int get_hp_base() const noexcept { return hpBase; }
	[[nodiscard]] int get_temp_hp() const noexcept { return tempHp; }
	[[nodiscard]] int get_effective_hp() const noexcept { return hp + tempHp; }
	// The fire and acid part of the damage taken, which only heal() can mend.
	[[nodiscard]] int get_unregenerable_damage() const noexcept { return unregenerableDamage; }
	[[nodiscard]] bool is_dead() const noexcept { return hp <= 0; }

	// Mutator methods
	void set_hp(int value) noexcept { hp = value; }
	void set_max_hp(int value) noexcept { hpMax = value; }
	void set_hp_base(int value) noexcept { hpBase = value; }
	void set_temp_hp(int value) noexcept { tempHp = std::max(0, value); }
	void add_temp_hp(int amount) noexcept { tempHp += amount; }
	// Restores the saved fire and acid damage; a load writes it after hp and hpMax.
	void set_unregenerable_damage(int value) noexcept { unregenerableDamage = value; }

	// Action methods
	int take_damage(Creature& owner, int damage, GameContext& ctx, DamageType damageType);

	// Heals up to hpToHeal and returns what it healed. Any healing but regeneration
	// takes the fire and acid damage first, so what is left is what regeneration can reach.
	//
	// Example, max 20 at 10 after 6 acid and 4 slashing:
	//   pool.heal(6);   // -> 6, hp 16, the acid healed
	[[nodiscard]] int heal(int hpToHeal);

	// Heals up to points of the damage regeneration can reach - never fire or acid,
	// never a creature already dead - and returns what it healed.
	//
	// Example, max 20 at 10 after 6 acid and 4 slashing:
	//   pool.regenerate(10);   // -> 4, hp 14; the acid waits for other healing
	[[nodiscard]] int regenerate(int points);
};
