#ifndef GOBLIN_H
#define GOBLIN_H

class Actor;

//==GOBLIN==
class Goblin : public Actor
{
public:
	Goblin(int y, int x);

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

#endif // GOBLIN_H