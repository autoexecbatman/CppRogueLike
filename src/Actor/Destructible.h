#pragma once

#include <string>
#include <string_view>

#include "../Persistent/Persistent.h"
#include "../Combat/DamageInfo.h"

class Creature;
class Player;
struct GameContext;

//====
class Destructible : public Persistent
{
private:
	int hpBase;  // Base HP without Constitution bonus
	int hpMax; // maximum health points
	int hp; // current health points
	int dr; // hit points deflected
	std::string corpseName; // the actor's name once it is dead/destroyed
	int xp; // for awarding experience points
	int thaco; // to hit armor class 0
	int armorClass; // armor class
	int baseArmorClass; // Store the base AC without equipment
	int lastConstitution; // Last known Constitution value to check for changes
	int tempHp; // Temporary hit points such as from aid spell

    [[nodiscard]] int calculate_dexterity_ac_bonus(const Creature& owner, GameContext& ctx) const;

    // Single source of truth: slot-based equipment AC via virtual get_equipped_item()
    // Players: returns items from slots, NPCs: returns nullptr (0 AC bonus)
	[[nodiscard]] int calculate_equipment_ac_bonus(const Creature& owner, GameContext& ctx) const;

    [[nodiscard]] int calculate_constitution_hp_bonus(const Creature& owner, GameContext& ctx) const;
    [[nodiscard]] int calculate_constitution_hp_bonus_for_value(int constitution, GameContext& ctx) const;
    [[nodiscard]] int calculate_level_multiplier(const Creature& owner) const;

    void handle_stat_drain_death(Creature& owner, GameContext& ctx);
    void log_constitution_change(const Creature& owner, GameContext& ctx, 
                                int oldCon, int newCon, int hpChange) const;
public:
	// Type enum for serialization - must be public for tests
	enum class DestructibleType { MONSTER, PLAYER };

	Destructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass);
	virtual ~Destructible() override = default;
	Destructible(const Destructible&) = delete;
	Destructible(Destructible&&) = delete;
	Destructible& operator=(const Destructible&) = delete;
	Destructible& operator=(Destructible&&) = delete;

	void update_constitution_bonus(Creature& owner, GameContext& ctx);

	// Query methods - const-correct
	[[nodiscard]] bool is_dead() const noexcept { return hp <= 0; }
	[[nodiscard]] int get_hp() const noexcept { return hp; }
	[[nodiscard]] int get_max_hp() const noexcept { return hpMax; }
	[[nodiscard]] int get_armor_class() const noexcept { return armorClass; }
	[[nodiscard]] int get_base_armor_class() const noexcept { return baseArmorClass; }
	[[nodiscard]] int get_thaco() const noexcept { return thaco; }
	[[nodiscard]] int get_dr() const noexcept { return dr; }
	[[nodiscard]] const std::string& get_corpse_name() const noexcept { return corpseName; }
	[[nodiscard]] int get_xp() const noexcept { return xp; }
	[[nodiscard]] int get_last_constitution() const noexcept { return lastConstitution; }
	[[nodiscard]] int get_hp_base() const noexcept { return hpBase; }
	// Temporary HP accessors
    [[nodiscard]] int get_temp_hp() const noexcept { return tempHp; }
    void set_temp_hp(int value) noexcept { tempHp = std::max(0, value); }
    void add_temp_hp(int amount) noexcept { tempHp += amount; }

    // Total effective HP (for display)
    [[nodiscard]] int get_effective_hp() const noexcept { return hp + tempHp; }

	// Modifier methods - non-const
	void set_hp(int value) noexcept { hp = value; }
	void set_max_hp(int value) noexcept { hpMax = value; }
	void set_hp_base(int value) noexcept { hpBase = value; }
	
	void set_armor_class(int value) noexcept { armorClass = value; }
	void set_base_armor_class(int value) noexcept { baseArmorClass = value; }
	void set_thaco(int value) noexcept { thaco = value; }
	void set_dr(int value) noexcept { dr = value; }
	void set_corpse_name(std::string_view name) { corpseName = name; }
	void set_xp(int value) noexcept { xp = value; }
	void set_last_constitution(int value) noexcept { lastConstitution = value; }
	
	void add_xp(int amount) noexcept { xp += amount; }
	
	// Action methods
	int take_damage(Creature& owner, int damage, GameContext& ctx, DamageType damageType = DamageType::PHYSICAL); // handles damage, owner attacked, returns (dam - def)
	virtual void die(Creature& owner, GameContext& ctx); // handles death, owner killed
	[[nodiscard]] int heal(int hpToHeal); // The function returns the amount of health point actually restored.
	void update_armor_class(Creature& owner, GameContext& ctx);

	void load(const json& j) override;
	void save(json& j) override;

	[[nodiscard]] static std::unique_ptr<Destructible> create(const json& j);
};

//====
class MonsterDestructible : public Destructible
{
public:
	MonsterDestructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass);
	void die(Creature& owner, GameContext& ctx) override;
	void save(json& j) override;
};

//====
class PlayerDestructible : public Destructible
{
public:
	PlayerDestructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass);
	void die(Creature& owner, GameContext& ctx) override;
	void save(json& j) override;
};
