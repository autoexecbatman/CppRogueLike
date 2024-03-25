// file: Ai.h
#ifndef AI_H
#define AI_H

#pragma once

#include <curses.h>

#include "Persistent.h"

class Actor; // for no circular dependency with Actor.h

//==AI==
class Ai : public Persistent
{
public:
	virtual ~Ai() {};

	virtual void update(Actor& owner) = 0;

	static std::unique_ptr<Ai> create(TCODZip& zip);

protected:
	enum class AiType
	{
		MONSTER, CONFUSED_MONSTER, PLAYER
	};
};

//====

#endif // !AI_H
// file: Ai.h
