// file: Ai.h
#ifndef AI_H
#define AI_H

#include <curses.h>

#include "Persistent.h"

class Actor; // for no circular dependency with Actor.h

//==AI==
class Ai : public Persistent
{
public:
	// Defaulted constructor and destructor
	Ai() = default;
	virtual ~Ai() {};

	// Defaulted copy constructor and copy assignment operator
	Ai(const Ai&) = default;
	Ai& operator=(const Ai&) = default;

	// Defaulted move constructor and move assignment operator
	Ai(Ai&&) noexcept = default;
	Ai& operator=(Ai&&) noexcept = default;

	virtual void update(Actor& owner) = 0;

	static std::shared_ptr<Ai> create(TCODZip& zip);

protected:
	enum class AiType
	{
		MONSTER, CONFUSED_MONSTER, PLAYER
	};
};

//====
class ConfusedMonsterAi : public Ai
{
public:
	ConfusedMonsterAi(int nbTurns, std::shared_ptr<Ai> oldAi);
	void update(Actor& owner);
	void load(TCODZip& zip);
	void save(TCODZip& zip);
	
protected:
	int nbTurns;
	std::shared_ptr<Ai> oldAi;
};

#endif // !AI_H
// file: Ai.h
