#include <libtcod.h>
#include "Confuser.h"
#include "../Actor/Actor.h"
#include "../Game.h"
#include "../Ai/AiMonsterConfused.h"

//==Confuser==
Confuser::Confuser(int nbTurns, int range) noexcept : nbTurns(nbTurns), range(range) {}

bool Confuser::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	//int x{ 0 }, y{ 0 }; // we modify these in pick_tile to get the target position
	Vector2D target{ 0, 0 };

	if (!ctx.targeting_system->pick_tile(ctx, &target, 0))
	{
		// CRITICAL FIX: Clear screen completely before restore
		clear();
		refresh();
		ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player, ctx);
		ctx.gui->gui_render(ctx);
		ctx.rendering_manager->force_screen_refresh();
		return false;
	}

	auto actor = ctx.map->get_actor(target, ctx); // get the actor at the target position

	if (!actor)
	{
		// CRITICAL FIX: Clear screen completely before restore
		clear();
		refresh();
		ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player, ctx);
		ctx.gui->gui_render(ctx);
		ctx.rendering_manager->force_screen_refresh();
		return false;
	}

	// replace the monster's AI with a confused one;
	// after <nbTurns> turns the old AI will be restored
	auto confusedAi = std::make_unique<AiMonsterConfused>(nbTurns, std::move(actor->ai));
	actor->ai = std::move(confusedAi);

	ctx.message_system->message(WHITE_BLACK_PAIR, std::format("The eyes of the {} look vacant,", actor->actorData.name), false);
	ctx.message_system->message(WHITE_BLACK_PAIR, " as he starts to stumble around!", true);

	// CRITICAL FIX: Clear screen completely before restore
	clear();
	refresh();
	ctx.rendering_manager->render_world(*ctx.map, *ctx.stairs, *ctx.objects, *ctx.inventory_data, *ctx.creatures, *ctx.player, ctx);
	ctx.gui->gui_render(ctx);
	ctx.rendering_manager->force_screen_refresh();
	return Pickable::use(owner, wearer, ctx);
}

void Confuser::load(const json& j)
{
	nbTurns = j["nbTurns"].get<int>();
	range = j["range"].get<int>();
}

void Confuser::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::CONFUSER);
	j["nbTurns"] = nbTurns;
	j["range"] = range;
}
