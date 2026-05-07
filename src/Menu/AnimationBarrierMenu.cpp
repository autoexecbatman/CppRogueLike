// file: AnimationBarrierMenu.cpp
#include <cassert>

#include "../Core/GameContext.h"
#include "../Gui/Gui.h"
#include "../Renderer/Renderer.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/RenderingManager.h"
#include "AnimationBarrierMenu.h"

AnimationBarrierMenu::AnimationBarrierMenu(GameContext& ctx)
{
	assert(ctx.renderer && "AnimationBarrierMenu constructed without renderer");
	assert(ctx.animSystem && "AnimationBarrierMenu constructed without animSystem");
	renderer = ctx.renderer;
}

void AnimationBarrierMenu::menu(GameContext& ctx)
{
	assert(ctx.renderer);
	assert(ctx.animSystem);
	assert(ctx.renderingManager);

	ctx.renderer->begin_frame();
	ctx.renderingManager->render(ctx);
	ctx.animSystem->update_and_render(*ctx.renderer);

	if (ctx.gui && ctx.gui->guiInit)
	{
		ctx.gui->gui_render(ctx);
	}

	ctx.renderer->end_frame();

	// Yield back to the menu stack once all particles have expired.
	if (!ctx.animSystem->has_active_entries())
	{
		run = false;
	}
}
