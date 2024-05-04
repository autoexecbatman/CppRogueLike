// file: AiMonster.h
#ifndef AI_MONSTER_H
#define AI_MONSTER_H

#pragma once

#include "Ai.h"
#include "../Persistent/Persistent.h"
#include "../Actor/Actor.h"

class AiMonster : public Ai
{
public:
	AiMonster();
	void update(Actor& owner) override;

	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

protected:
	int moveCount = 0;

	void moveOrAttack(Actor& owner, int targetx, int targety);
};

#endif // !AI_MONSTER_H
// file: AiMonster.h
