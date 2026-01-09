// file Destructible.h
#ifndef DESTRUCTIBLE_H
#define DESTRUCTIBLE_H

#include "../Persistent/Persistent.h"

class Creature;
struct GameContext;

//====
class Destructible : public Persistent
{
private:
	int lastConstitution{ 0 }; // Last known Constitution value to check for changes
	int hpBase{ 0 };  // Base HP without Constitution bonus
	int hpMax{ 0 }; // maximum health points
	int hp{ 0 }; // current health points
	int dr{ 0 }; // hit points deflected
	std::string corpseName{ "corpseName" }; // the actor's name once it is dead/destroyed
	int xp{ 0 }; // for awarding experience points
	int thaco{ 0 }; // to hit armor class 0
	int armorClass{ 0 }; // armor class
	int baseArmorClass{ 0 }; // Store the base AC without equipment

public:
	Destructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass);
	void update_constitution_bonus(Creature& owner, GameContext& ctx);
	virtual ~Destructible() override = default;
	Destructible(const Destructible&) = delete;
	Destructible(Destructible&&) = delete;
	Destructible& operator=(const Destructible&) = delete;
	Destructible& operator=(Destructible&&) = delete;

	// Query methods - const-correct
	bool is_dead() const noexcept { return hp <= 0; }
	int get_hp() const noexcept { return hp; }
	int get_max_hp() const noexcept { return hpMax; }
	int get_armor_class() const noexcept { return armorClass; }
	int get_base_armor_class() const noexcept { return baseArmorClass; }
	int get_thaco() const noexcept { return thaco; }
	int get_dr() const noexcept { return dr; }
	const std::string& get_corpse_name() const noexcept { return corpseName; }
	int get_xp() const noexcept { return xp; }
	int get_last_constitution() const noexcept { return lastConstitution; }
	int get_hp_base() const noexcept { return hpBase; }

	// Modifier methods - non-const
	void set_hp(int value) noexcept 
	{
		hp = value;
		if (hp < 0) hp = 0;
		if (hp > hpMax) hp = hpMax;
	}
	
	void set_max_hp(int value) noexcept 
	{
		hpMax = value;
		if (hpMax <= 0) hpMax = 1;
		if (hp > hpMax) hp = hpMax;
	}
	
	void set_hp_base(int value) noexcept 
	{
		hpBase = value;
		if (hpBase <= 0) hpBase = 1;
	}
	
	void set_armor_class(int value) noexcept { armorClass = value; }
	void set_base_armor_class(int value) noexcept { baseArmorClass = value; }
	void set_thaco(int value) noexcept { thaco = value; }
	void set_dr(int value) noexcept { dr = value; }
	void set_corpse_name(std::string_view name) { corpseName = name; }
	void set_xp(int value) noexcept { xp = value; }
	
	void add_xp(int amount) noexcept { xp += amount; }
	
	void set_last_constitution(int value) noexcept { lastConstitution = value; }

	// Action methods
	void take_damage(Creature& owner, int damage, GameContext& ctx); // handles damage, owner attacked, returns (dam - def)
	virtual void die(Creature& owner, GameContext& ctx); // handles death, owner killed
	int heal(int hpToHeal); // The function returns the amount of health point actually restored.
	void update_armor_class(Creature& owner, GameContext& ctx);

	void load(const json& j) override;
	void save(json& j) override;

	static std::unique_ptr<Destructible> create(const json& j);
	
protected:
	enum class DestructibleType
	{
		MONSTER, PLAYER
	};
};

//====
class MonsterDestructible : public Destructible
{
public:
	MonsterDestructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass);
	void die(Creature& owner, GameContext& ctx) override;
	void save(json& j);
};

//====
class PlayerDestructible : public Destructible
{
public:
	PlayerDestructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass);
	void die(Creature& owner, GameContext& ctx) override;
	void save(json& j);
};

#endif // !DESTRUCTIBLE_H
// end of file: Destructible.h
