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
	[[nodiscard]] bool is_dead() const noexcept { return hp <= 0; }

	// Mutator methods
	void set_hp(int value) noexcept { hp = value; }
	void set_max_hp(int value) noexcept { hpMax = value; }
	void set_hp_base(int value) noexcept { hpBase = value; }
	void set_temp_hp(int value) noexcept { tempHp = std::max(0, value); }
	void add_temp_hp(int amount) noexcept { tempHp += amount; }

	// Action methods
	int take_damage(Creature& owner, int damage, GameContext& ctx, DamageType damageType);
	[[nodiscard]] int heal(int hpToHeal);
};
