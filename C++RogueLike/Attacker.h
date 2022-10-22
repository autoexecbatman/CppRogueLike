#pragma once

class Attacker : public Persistent
{
public:

	float power = 0; // hit points given
	
	Attacker(float power);

	void attack(Actor* owner, Actor* target);
	
	void load(TCODZip& zip);
	void save(TCODZip& zip);
};