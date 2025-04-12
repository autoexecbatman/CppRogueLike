// file Destructible.h
#ifndef DESTRUCTIBLE_H
#define DESTRUCTIBLE_H

#include "../Persistent/Persistent.h"

class Creature;

//====
class Destructible : public Persistent
{
private:
	int lastConstitution{ 0 }; // Last known Constitution value to check for changes
public:
	int hpBase{ 0 };  // Base HP without Constitution bonus
	int hpMax{ 0 }; // maximum health points
	int hp{ 0 }; // current health points
	int dr{ 0 }; // hit points deflected
	std::string corpseName{ "corpseName" }; // the actor's name once it is dead/destroyed
	int xp{ 0 }; // for awarding experience points
	int thaco{ 0 }; // to hit armor class 0
	int armorClass{ 0 }; // armor class

	Destructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass);
	void update_constitution_bonus(Creature& owner);
	virtual ~Destructible() override = default;
	Destructible(const Destructible&) = delete;
	Destructible(Destructible&&) = delete;
	Destructible& operator=(const Destructible&) = delete;
	Destructible& operator=(Destructible&&) = delete;

	bool is_dead() const noexcept { return hp <= 0; }
	void take_damage(Creature& owner, int damage); // handles damage, owner attacked, returns (dam - def)
	virtual void die(Creature& owner); // handles death, owner killed
	int heal(int hpToHeal); // The function returns the amount of health point actually restored.

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
	void die(Creature& owner) override;
	void save(json& j);
};
//====
class PlayerDestructible : public Destructible
{
public:
	PlayerDestructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass);
	void die(Creature& owner) override;
	void save(json& j);
};
//====

#endif // !DESTRUCTIBLE_H
// end of file: Destructible.h
