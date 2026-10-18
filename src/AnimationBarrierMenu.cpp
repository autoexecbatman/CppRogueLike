// file: AnimationBarrierMenu.cpp
#include <cassert>

#include "AnimationBarrierMenu.h"
#include "AnimationSystem.h"
#include "GameContext.h"
#include "Gui.h"
#include "Renderer.h"
#include "RenderingManager.h"

AnimationBarrierMenu::AnimationBarrierMenu(GameContext& ctx)
{
	assert(ctx.renderer && "AnimationBarrierMenu constructed without renderer");
	assert(ctx.animSystem && "AnimationBarrierMenu constructed without animSystem");
	renderer = ctx.renderer;
}

void AnimationBarrierMenu::menu(GameContext& ctx)
{
	assert(ctx.gui && "AnimationBarrierMenu::menu: called without a gui");
	assert(ctx.renderer);
	assert(ctx.animSystem);
	assert(ctx.renderingManager);

	ctx.renderingManager->render(ctx);
	ctx.animSystem->update_and_render(*ctx.renderer);

	if (ctx.gui->guiInit)
	{
		ctx.gui->gui_render(ctx);
	}

	// Yield back to the menu stack once all particles have expired.
	if (!ctx.animSystem->has_active_entries())
	{
		run = false;
	}
}
