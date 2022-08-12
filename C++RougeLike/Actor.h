#pragma once

//a class for the entities named Actor
class Actor
{
public:
	//add hp
	int hp = 100;
	//add name
	const char* name;
	
	//position on map
	int y = 0, x = 0;

	//the symbol to print
	int ch = -47;

	//color for the actor
	int col = 0;

	//We're storing the actor's name in a const char pointer.
	//the actor's name
	/*const char* name = "Actor";*/
	
	//does the actor blocks movement?
	bool blocks = false;

	//constructor declaration
	Actor(int y, int x, int ch, const char* name, int col);
	
	//update() will handle the monster turn.
	void update();
	//moveOrAttack() will handle the player move.
	//It returns true if the player actually moved, false it he hits a wall or another creature.
	bool moveOrAttack(int x, int y);
	
	//render the actor on the screen.
	void render() const;
};

//====
//make an Attacker class that has hit points
class Attacker
{
	public:
	float power = 0;//hit points
	Attacker(float power);
	void attack(Actor* owner, Actor* target);
};
//====

class Destructible
{
	float maxHp;//maximum hit points
	float hp;//current hit points
	float defense;//defense
	const char* corpseName;//the name of the corpse
	
	Destructible(float maxHp,float hp, float defense, const char* corpseName);
	inline bool isDead() { return hp <= 0; }
	float takeDamage(Actor* owner, float damage);
	virtual void die(Actor* owner);
};