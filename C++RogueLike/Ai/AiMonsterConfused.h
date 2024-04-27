// file: AiMonsterConfused.h
#ifndef AI_MONSTER_CONFUSED_H
#define AI_MONSTER_CONFUSED_H

#include <memory>
#include <libtcod.h>

#include "Actor.h"
#include "Ai.h"

class AiMonsterConfused : public Ai
{
public:
	AiMonsterConfused(int nbTurns, std::unique_ptr<Ai> oldAi) noexcept;
	void update(Actor& owner) override;
	void load(TCODZip& zip) override;
	void save(TCODZip& zip) override;

protected:
	int nbTurns;
	std::unique_ptr<Ai> oldAi;
};

#endif // !AI_MONSTER_CONFUSED_H
