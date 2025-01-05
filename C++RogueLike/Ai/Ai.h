// file: Ai.h
#ifndef AI_H
#define AI_H

#pragma once

#include <curses.h>

#include "../Persistent/Persistent.h"

class Creature; // for no circular dependency with Creature.h

//==AI==
class Ai : public Persistent
{
public:
	virtual ~Ai() {};

	virtual void update(Creature& owner) = 0;

	static std::unique_ptr<Ai> create(const json& j);

protected:
	enum class AiType
	{
		MONSTER, CONFUSED_MONSTER, PLAYER, SHOPKEEPER
	};
};

//====

#endif // !AI_H
// file: Ai.h
