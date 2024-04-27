// file: Player.h
#ifndef PLAYER_H
#define PLAYER_H

#include <gsl/util>
#include "RandomDice.h"
#include "Map.h"
#include "Actor.h"

class Player : public Actor
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
	int playerGold{ 0 };

	Player(int y, int x, int maxHp, int dr, std::string corpseName, int xp, int thaco, int armorClass, int dmg, int minDmg, int maxDmg, bool canSwim);

	// Note::
	// X/Y coordinates set
	// in the function create_room()
	// in Map.cpp
	void setPosX(int x) noexcept { posX = x; }
	void setPosY(int y) noexcept { posY = y; }
	int getPosX() const noexcept { return posX; }
	int getPosY() const noexcept { return posY; }
	void player_get_pos_from_map();

	void racial_ability_adjustments();
	void calculate_thaco();

	std::string get_weapon_equipped() const noexcept { return weaponEquipped; }
	void set_weapon_equipped(std::string weapon) noexcept { weaponEquipped = weapon; }
};

#endif // !PLAYER_H
// end of file: Player.h
