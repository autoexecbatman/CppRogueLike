#include <cassert>
#include <cmath>
#include <memory>
#include <vector>

#include <raylib.h>

#include "../Actor/Actor.h"
#include "../Actor/EquipmentSlot.h"
#include "../ActorTypes/Player.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../Items/ItemClassification.h"
#include "../Map/Map.h"
#include "../Renderer/Renderer.h"
#include "AnimationSystem.h"
#include "CreatureManager.h"
#include "MessageSystem.h"
#include "RenderingManager.h"
#include "TileConfig.h"
#include "TargetingMenu.h"
#include "../Utils/Vector2D.h"
#include "TargetingSystem.h"

void TargetingSystem::draw_los(GameContext& ctx, Vector2D targetCursor) const
{
	if (!ctx.renderer || !ctx.map || !ctx.player)
	{
		return;
	}

	auto path = Map::bresenham_line(ctx.player->position, targetCursor);
	int tileSize = ctx.renderer->get_tile_size();
	int cameraOffsetX = ctx.renderer->get_camera_x();
	int cameraOffsetY = ctx.renderer->get_camera_y();

	bool hasLineOfSight = ctx.map->has_los(ctx.player->position, targetCursor);

	for (const auto& pos : path)
	{
		if (pos == ctx.player->position)
		{
			continue;
		}

		int screenX = pos.x * tileSize - cameraOffsetX;
		int screenY = pos.y * tileSize - cameraOffsetY;
		Color tint = hasLineOfSight ? Color{ 50, 220, 100, 50 } : Color{ 220, 50, 50, 50 };
		DrawRectangle(screenX, screenY, tileSize, tileSize, tint);
	}
}

void TargetingSystem::draw_range_indicator(GameContext& ctx, Vector2D center, int range) const
{
	if (!ctx.renderer || !ctx.map || range <= 0)
	{
		return;
	}

	int tileSize = ctx.renderer->get_tile_size();
	int cameraOffsetX = ctx.renderer->get_camera_x();
	int cameraOffsetY = ctx.renderer->get_camera_y();

	for (int dy = -range; dy <= range; ++dy)
	{
		for (int dx = -range; dx <= range; ++dx)
		{
			Vector2D pos{ center.x + dx, center.y + dy };
			if (!ctx.map->is_in_bounds(pos))
			{
				continue;
			}

			double distanceToCenter = pos.distance_to(center);
			if (distanceToCenter < static_cast<double>(range) - 0.5)
			{
				continue;
			}
			if (distanceToCenter > static_cast<double>(range) + 0.5)
			{
				continue;
			}

			int screenX = pos.x * tileSize - cameraOffsetX;
			int screenY = pos.y * tileSize - cameraOffsetY;
			DrawRectangle(screenX, screenY, tileSize, tileSize, Color{ 100, 200, 255, 35 });
		}
	}
}

bool TargetingSystem::is_valid_target(GameContext& ctx, Vector2D from, Vector2D to, int maxRange) const
{
	if (from.distance_to(to) > maxRange)
	{
		return false;
	}

	if (!ctx.map->has_los(from, to))
	{
		return false;
	}

	auto entity = ctx.map->get_actor(to, ctx);
	if (!entity)
	{
		return false;
	}

	if (entity == ctx.player)
	{
		return false;
	}

	return true;
}

void TargetingSystem::draw_aoe_preview(GameContext& ctx, Vector2D center, int radius) const
{
	if (!ctx.renderer || !ctx.map || radius <= 0)
	{
		return;
	}

	int tileSize = ctx.renderer->get_tile_size();
	int cameraOffsetX = ctx.renderer->get_camera_x();
	int cameraOffsetY = ctx.renderer->get_camera_y();

	for (int dy = -radius; dy <= radius; ++dy)
	{
		for (int dx = -radius; dx <= radius; ++dx)
		{
			Vector2D pos{ center.x + dx, center.y + dy };
			if (!ctx.map->is_in_bounds(pos))
			{
				continue;
			}
			if (pos.distance_to(center) > static_cast<double>(radius))
			{
				continue;
			}

			int screenX = pos.x * tileSize - cameraOffsetX;
			int screenY = pos.y * tileSize - cameraOffsetY;
			DrawRectangle(screenX, screenY, tileSize, tileSize, Color{ 255, 140, 0, 50 });
		}
	}
}

void TargetingSystem::handle_ranged_attack(GameContext& ctx) const
{
	if (!ctx.player->has_state(ActorState::IS_RANGED))
	{
		ctx.messageSystem->message(WHITE_BLACK_PAIR, "You need a ranged weapon to attack at a distance!", true);
		return;
	}

	Item* missileWeapon = ctx.player->get_equipped_item(EquipmentSlot::MISSILE_WEAPON);
	const int weaponRange = get_weapon_range(missileWeapon);

	auto onTarget = [](bool confirmed, Vector2D targetPos, GameContext& innerCtx)
	{
		if (!confirmed)
		{
			return;
		}

		Creature* target = innerCtx.map->get_actor(targetPos, innerCtx);
		if (!target || target == innerCtx.player)
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "No valid target there.", true);
			return;
		}

		if (!innerCtx.map->has_los(innerCtx.player->position, targetPos))
		{
			innerCtx.messageSystem->message(WHITE_BLACK_PAIR, "No clear line of sight.", true);
			return;
		}

		if (innerCtx.animSystem && innerCtx.tileConfig)
		{
			const TileRef boltTile = innerCtx.tileConfig->get("TILE_EFFECT_BOLT");

			auto onArrive = [targetPos, &innerCtx]()
			{
				if (innerCtx.animSystem)
				{
					innerCtx.animSystem->spawn_blood_burst(targetPos.x, targetPos.y, 3);
				}
			};

			innerCtx.animSystem->spawn_projectile(
				innerCtx.player->position,
				targetPos,
				boltTile,
				210, 180, 100,
				550.0f,
				0.0f,
				std::move(onArrive));
		}

		assert(innerCtx.player->attacker && "ranged attack fired with no attacker component");
		assert(innerCtx.creatureManager && "ranged attack fired with no creature manager");
		innerCtx.player->attacker->attack(*target, innerCtx);
		innerCtx.creatureManager->cleanup_dead_creatures(*innerCtx.creatures);
		innerCtx.gameState->set_game_status(GameStatus::NEW_TURN);
	};

	ctx.menus->push_back(std::make_unique<TargetingMenu>(weaponRange, 0, std::move(onTarget), ctx));
}

TargetResult TargetingSystem::acquire_targets(
	GameContext& ctx,
	TargetMode mode,
	Vector2D origin,
	int range,
	int aoe_radius) const
{
	switch (mode)
	{

	case TargetMode::AUTO_NEAREST:
	{
		return target_auto_nearest(ctx, origin, range);
	}

	case TargetMode::PICK_TILE_SINGLE:
	case TargetMode::PICK_TILE_AOE:
	case TargetMode::FOV_BUFF:
	{
		return {};
	}

	}

	return {};
}

int TargetingSystem::get_weapon_range(const Item* weapon)
{
	if (!weapon)
	{
		return 4;
	}

	switch (weapon->itemClass)
	{

	case ItemClass::BOW:
	{
		if (weapon->itemKey == "long_bow")
		{
			return 7;
		}
		else
		{
			return 5;
		}
	}

	case ItemClass::CROSSBOW:
	{
		return 6;
	}

	default:
	{
		return 4;
	}

	}
}

TargetResult TargetingSystem::target_auto_nearest(GameContext& ctx, Vector2D origin, int range) const
{
	const auto& monster = ctx.creatureManager->get_closest_monster(*ctx.creatures, origin, range);
	if (!monster)
	{
		return {};
	}
	return { true, { monster } };
}
