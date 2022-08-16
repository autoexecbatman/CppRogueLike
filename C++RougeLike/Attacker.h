#pragma once

class Attacker
{
public:
	
	float power = 0;//hit points given
	
	Attacker(float power);
	void attack(Actor* owner, Actor* target);

private:

};