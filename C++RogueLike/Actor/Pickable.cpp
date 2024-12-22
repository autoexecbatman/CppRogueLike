// file: Pickable.cpp
#include <gsl/util>
#include <vector>

#include "Pickable.h"
#include "../Colors/Colors.h"
#include "../Game.h"
#include "Actor.h"
#include "Confuser.h"
#include "Container.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../Ai/AiMonsterConfused.h"

//==PICKABLE==
bool Pickable::use(Item& owner, Creature& wearer)
{
	if (wearer.container)
	{
		auto compareItems = [&owner](const std::unique_ptr<Item>& actor) { return actor.get() == &owner; };
		std::erase_if(wearer.container->inv, compareItems);

		return true;
	}

	return false;
}

std::unique_ptr<Pickable> Pickable::create(TCODZip& zip)
{
	const PickableType type = static_cast<PickableType>(zip.getInt());
	std::unique_ptr<Pickable> pickable = nullptr;

	switch (type)
	{

	case PickableType::HEALER:
	{
		pickable = std::make_unique<Healer>(0);
		break;
	}

	case PickableType::LIGHTNING_BOLT:
	{
		pickable = std::make_unique<LightningBolt>(0, 0);
		break;
	}

	case PickableType::CONFUSER:
	{
		pickable = std::make_unique<Confuser>(0, 0);
		break;
	}

	case PickableType::FIREBALL:
	{
		pickable = std::make_unique<Fireball>(0, 0);
		break;
	}

	case PickableType::LONGSWORD:
	{
		pickable = std::make_unique<LongSword>();
		break;
	}

	case PickableType::DAGGER:
	{
		pickable = std::make_unique<Dagger>();
		break;
	}

	case PickableType::SHORTSWORD:
	{
		pickable = std::make_unique<ShortSword>();
		break;
	}

	case PickableType::LONGBOW:
	{
		pickable = std::make_unique<Longbow>();
		break;
	}

	case PickableType::STAFF:
	{
		pickable = std::make_unique<Staff>();
		break;
	}

	} // end of switch (type)

	if (!pickable) {
		game.log("Pickable create() failed. No pickable type found.");
		game.err("Pickable create() failed. No pickable type found.");
		return nullptr;
	}
	pickable->load(zip);

	return pickable;
}

bool Dagger::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	// unequip the weapon
	else
	{
		wearer.attacker->roll = this->roll;
		wearer.unequip(owner);
	}

	return false;
}

void Dagger::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::DAGGER));
	zip.putString(roll.data());
}

void Dagger::load(TCODZip& zip)
{
	this->roll = zip.getString();
}

bool LongSword::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	else
	{
		wearer.attacker->roll = this->roll;
		wearer.unequip(owner);
	}

	return false;
}

void LongSword::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::LONGSWORD));
	zip.putString(roll.data());
}

void LongSword::load(TCODZip& zip)
{
	this->roll = zip.getString();
}

// end of file: Pickable.cpp

bool ShortSword::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	// unequip the weapon
	else
	{
		wearer.attacker->roll = this->roll;
		wearer.unequip(owner);
	}

	return false;
}

void ShortSword::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::SHORTSWORD));
	zip.putString(roll.data());
}

void ShortSword::load(TCODZip& zip)
{
	this->roll = zip.getString();
}

bool Longbow::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	// unequip the weapon
	else
	{
		wearer.attacker->roll = this->roll;
		wearer.unequip(owner);
	}
	return false;
}

void Longbow::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::LONGBOW));
	zip.putString(roll.data());
}

void Longbow::load(TCODZip& zip)
{
	this->roll = zip.getString();
}

bool Staff::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	// unequip the weapon
	else
	{
		wearer.attacker->roll = this->roll;
		wearer.unequip(owner);
	}
	return false;
}

void Staff::save(TCODZip& zip)
{
	zip.putInt(static_cast<std::underlying_type_t<PickableType>>(PickableType::STAFF));
	zip.putString(roll.data());
}

void Staff::load(TCODZip& zip)
{
	this->roll = zip.getString();
}
