#pragma once

#include <string>
#include <string_view>

class Creature;
struct GameContext;

// One equipped item's contribution to armour class. An empty name means the
// slot contributed nothing, so a reader can skip it without a second flag.
struct ArmorClassSource
{
	std::string_view name{};
	int bonus{ 0 };
};

// Everything update() worked out on its way to a number. Returned rather than
// logged, so the calculation does not also decide who is told about it: in AD&D
// 2e a lower armour class is better, which is why the bonuses are negative.
struct ArmorClassBreakdown
{
	int previous{ 0 };
	int total{ 0 };
	int base{ 0 };
	int dexterity{ 0 };
	int equipment{ 0 };
	int temporary{ 0 };
	bool changed{ false };
	ArmorClassSource armor{};
	ArmorClassSource shield{};
	ArmorClassSource ring{};
	ArmorClassSource helm{};
};

class ArmorClass
{
private:
	int armorClass{};
	int baseArmorClass{};

	[[nodiscard]] int calculate_dexterity_ac_bonus(const Creature& owner, GameContext& ctx) const;
	[[nodiscard]] int calculate_equipment_ac_bonus(const Creature& owner, ArmorClassBreakdown& breakdown) const;

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

	// Recomputes and stores the armour class, returning how it was reached.
	// Reports rather than announces: the caller decides whether the breakdown is
	// worth telling anyone about.
	ArmorClassBreakdown update(Creature& owner, GameContext& ctx);
};
