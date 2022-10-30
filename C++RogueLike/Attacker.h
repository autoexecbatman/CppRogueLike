#ifndef PROJECT_PATH_ATTACKER_H_
#define PROJECT_PATH_ATTACKER_H_

#include "libtcod.hpp"
#include "Persistent.h"

class Actor;

class Attacker : public Persistent
{
public:

	int power = 0; // hit points given
	
	Attacker(int power);

	void attack(Actor* owner, Actor* target);
	
	void load(TCODZip& zip);
	void save(TCODZip& zip);
};

#endif // !PROJECT_PATH_ATTACKER_H_