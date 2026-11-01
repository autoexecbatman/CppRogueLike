#pragma once

#include "BaseMenu.h"

namespace CppRL
{

	struct GameContext;

	// Sits on top of the menu stack while the AnimationSystem has active
	// particles. Renders the full game world and animations each frame so
	// effects fired from menu context (e.g. a scroll of fireball used from
	// inventory) are visible. Pops itself the moment all particles and
	// projectiles have expired.
	class AnimationBarrierMenu : public BaseMenu
	{
	public:
		explicit AnimationBarrierMenu(GameContext& ctx);
		void menu(GameContext& ctx) override;
	};

} // namespace CppRL
