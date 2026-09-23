#pragma once

#include "AiSpider.h"

class Creature;
struct GameContext;

// The game's giant spider, which is the Monstrous Manual's huge spider (PDF page 1878):
// a hunting spider, 1-6 bite, THAC0 19. Its poison is Type A, and its victims save at +1.
class AiGiantSpider : public AiSpider
{
public:
	explicit AiGiantSpider(int poisonChance);

	[[nodiscard]] AiType get_ai_type() const noexcept override { return AiType::GIANT_SPIDER; }

protected:
	// Type A: 15 points fifteen rounds after the bite, or nothing on a save at +1.
	void inject_venom(Creature& owner, Creature& target, GameContext& ctx) override;
};
