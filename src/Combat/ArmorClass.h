#pragma once

#include <string>

class Creature;
struct GameContext;

class ArmorClass
{
private:
	int armorClass{};
	int baseArmorClass{};

	[[nodiscard]] int calculate_dexterity_ac_bonus(const Creature& owner, GameContext& ctx) const;
	[[nodiscard]] int calculate_equipment_ac_bonus(const Creature& owner, GameContext& ctx) const;

public:
	ArmorClass(int baseAC);
	~ArmorClass() = default;
	ArmorClass(const ArmorClass&) = delete;
	ArmorClass(ArmorClass&&) = delete;
	ArmorClass& operator=(const ArmorClass&) = delete;
	ArmorClass& operator=(ArmorClass&&) = delete;

	[[nodiscard]] int get_armor_class() const noexcept { return armorClass; }
	[[nodiscard]] int get_base_armor_class() const noexcept { return baseArmorClass; }

	void set_armor_class(int value) noexcept { armorClass = value; }
	void set_base_armor_class(int value) noexcept { baseArmorClass = value; }

	void update(Creature& owner, GameContext& ctx);
};
