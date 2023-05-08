// file: Attacker.h
#ifndef ATTACKER_H
#define ATTACKER_H

//#include "libtcod.hpp"
#pragma warning (push, 0)
#include <libtcod/libtcod.hpp>
#pragma warning (pop)
#include "Persistent.h"

class Actor;

class Attacker : public Persistent
{
public:

	int power = 0; // hit points given
	
	Attacker(int power) noexcept;

	void attack(const Actor& owner, Actor& target);
	
	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;
};

#endif // !ATTACKER_H
// file: Attacker.h
