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
	
	Attacker(int power);

	void attack(Actor& owner, Actor& target);
	
	void load(TCODZip& zip);
	void save(TCODZip& zip);
};

#endif // !ATTACKER_H
// file: Attacker.h
