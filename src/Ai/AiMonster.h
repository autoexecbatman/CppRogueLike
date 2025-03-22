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
	void update(Creature& owner) override;

	void load(const json& j) override;
	void save(json& j) override;

protected:
	int moveCount = 0;

	void moveOrAttack(Creature& owner, Vector2D position);
};

#endif // !AI_MONSTER_H
// file: AiMonster.h
