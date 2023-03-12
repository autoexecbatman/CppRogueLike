#ifndef PROJECT_PATH_GOBLIN_H_
#define PROJECT_PATH_GOBLIN_H_

class Actor;

//==GOBLIN==
class Goblin : public Actor
{
public:
	/*Goblin(int y, int x);*/
	Goblin(int y, int x);

	/*Actor* create_goblin(int y, int x);*/

	// make a create_goblin function that returns a std::unique_ptr<Actor> instead of an Actor*

	std::shared_ptr<Goblin> create_goblin(int y, int x);

};
//====

//==ORC==
class Orc : Actor
{
public:
	Orc(int y, int x);
	~Orc();

	std::shared_ptr<Actor> create_orc(int y, int x);
};
//====

//==TROLL===
class Troll : Actor
{
public:
	Troll(int y, int x);
	~Troll();

	std::shared_ptr<Actor> create_troll(int y, int x);
};
//====

//==DRAGON===
class Dragon : Actor
{
public:
	Dragon(int y, int x);
	~Dragon();

	std::shared_ptr<Actor> create_dragon(int y, int x);
};
//====

#endif // PROJECT_PATH_GOBLIN_H_