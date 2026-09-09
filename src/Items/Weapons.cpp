#include <string>

#include "Weapons.h"

std::string Weapons::get_damage_roll(bool twoHanded) const noexcept
{
	if (twoHanded && !damageRollTwoHanded.empty())
	{
		return damageRollTwoHanded;
	}
	return damageRoll;
}
