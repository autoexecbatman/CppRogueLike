// file: Player.h
#ifndef PLAYER_H
#define PLAYER_H

#include "../Random/RandomDice.h"
#include "../Map/Map.h"
#include "../Actor/Actor.h"



class Player : public Creature
{
public:
	enum class PlayerRaceState : int
	{
		NONE,
		HUMAN,
		ELF,
		DWARF,
		HALFLING,
		GNOME,
		HALFELF
	} playerRaceState{ PlayerRaceState::NONE };

	enum class PlayerClassState : int
	{
		NONE,
		FIGHTER,
		ROGUE,
		CLERIC,
		WIZARD
	} playerClassState{ PlayerClassState::NONE };

	std::string playerGender{ "None" };
	int playerGold{ 100 };

	Player(Vector2D position, int maxHp, int dr, std::string corpseName, int xp, int thaco, int armorClass, int dmg, int minDmg, int maxDmg);

	// Note::
	// X/Y coordinates set
	// in the function create_room()
	// in Map.cpp

	void setXY(Vector2D pos) noexcept { position = pos; }
	Vector2D getXY() const noexcept { return position; }

	void racial_ability_adjustments();
	void calculate_thaco();

	std::string get_weapon_equipped() const noexcept { return weaponEquipped; }
	void set_weapon_equipped(std::string weapon) noexcept { weaponEquipped = weapon; }
};

#endif // !PLAYER_H
// end of file: Player.h
