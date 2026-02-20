#include <algorithm>
#include <cmath>
#include <ranges>

#include "TargetingSystem.h"
#include "../Core/GameContext.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/InputSystem.h"
#include "../Systems/CreatureManager.h"
#include "../Systems/MessageSystem.h"
#include "../ActorTypes/Player.h"
#include "../Systems/InputHandler.h"
#include "../Systems/RenderingManager.h"
#include "../Systems/DataManager.h"
#include "../Items/ItemClassification.h"
#include "../Colors/Colors.h"
#include "../Gui/Gui.h"

const Vector2D TargetingSystem::select_target(GameContext& ctx, Vector2D startPos, int maxRange) const
{
	// Check if player is trying to attack but doesn't have a ranged weapon
	if (!ctx.player->has_state(ActorState::IS_RANGED))
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "You need a ranged weapon to attack at a distance!", true);
		return Vector2D{ -1, -1 };
	}

	// Build sorted list of valid targets (visible + in range + LOS)
	std::vector<Creature*> valid_targets;
	for (const auto& creature : *ctx.creatures)
	{
		if (!creature || creature->destructible->is_dead())
			continue;
		if (!ctx.map->is_in_fov(creature->position))
			continue;
		if (!is_valid_target(ctx, startPos, creature->position, maxRange))
			continue;
		valid_targets.push_back(creature.get());
	}

	auto by_distance = [&startPos](const Creature* a, const Creature* b)
	{
		return a->get_tile_distance(startPos) < b->get_tile_distance(startPos);
	};
	std::ranges::sort(valid_targets, by_distance);

	// Start on the nearest target, or player position if no targets in range
	int target_index = 0;
	Vector2D targetCursor = valid_targets.empty()
		? ctx.player->position
		: valid_targets[0]->position;

	// TODO: stub - targeting UI loop requires curses replacement
	// Previously used clear/render/mvchgat/mvaddch/mvprintw/attron/attroff/refresh/getch
	// Key constants: UP=0x103, DOWN=0x102, LEFT=0x104, RIGHT=0x105
	// Game logic for target cycling (Tab/Shift+Tab), fine aim (arrow keys),
	// fire (Enter), cancel (Esc) needs to be reimplemented with new renderer

	// For now, auto-select nearest target if available
	if (!valid_targets.empty())
	{
		targetCursor = valid_targets[0]->position;
		if (is_valid_target(ctx, startPos, targetCursor, maxRange))
		{
			if (ctx.map->is_in_fov(targetCursor))
			{
				const auto& actor = ctx.map->get_actor(targetCursor, ctx);
				if (actor)
				{
					ctx.player->attacker->attack(*ctx.player, *actor, ctx);
					*ctx.game_status = GameStatus::NEW_TURN;
					return targetCursor;
				}
			}
		}
	}

	return Vector2D{ -1, -1 };
}

void TargetingSystem::draw_los(GameContext& ctx, Vector2D targetCursor) const
{
	if (!ctx.renderer || !ctx.map || !ctx.player) return;

	auto path = Map::bresenham_line(ctx.player->position, targetCursor);
	int  ts    = ctx.renderer->get_tile_size();
	int  cam_x = ctx.renderer->get_camera_x();
	int  cam_y = ctx.renderer->get_camera_y();

	bool clear = ctx.map->has_los(ctx.player->position, targetCursor);

	for (const auto& pos : path)
	{
		if (pos == ctx.player->position) continue;
		int sx = pos.x * ts - cam_x;
		int sy = pos.y * ts - cam_y;
		// Green tint = clear LOS, red = blocked
		Color c = clear
			? Color{50, 220, 100, 50}
			: Color{220, 50,  50,  50};
		DrawRectangle(sx, sy, ts, ts, c);
	}
}

void TargetingSystem::draw_range_indicator(GameContext& ctx, Vector2D center, int range) const
{
	if (!ctx.renderer || !ctx.map || range <= 0) return;

	int ts    = ctx.renderer->get_tile_size();
	int cam_x = ctx.renderer->get_camera_x();
	int cam_y = ctx.renderer->get_camera_y();

	for (int dy = -range; dy <= range; ++dy)
	{
		for (int dx = -range; dx <= range; ++dx)
		{
			Vector2D pos{ center.x + dx, center.y + dy };
			if (!ctx.map->is_in_bounds(pos)) continue;

			double dist = pos.distance_to(center);
			// Draw only tiles near the edge of the range (ring, not filled circle)
			if (dist < static_cast<double>(range) - 0.5) continue;
			if (dist > static_cast<double>(range) + 0.5) continue;

			int sx = pos.x * ts - cam_x;
			int sy = pos.y * ts - cam_y;
			DrawRectangle(sx, sy, ts, ts, Color{100, 200, 255, 35});
		}
	}
}

// Check if a target position is valid
bool TargetingSystem::is_valid_target(GameContext& ctx, Vector2D from, Vector2D to, int maxRange) const
{
	// Check if the target is within range
	if (from.distance_to(to) > maxRange)
	{
		return false;
	}

	// Check if there's a clear line of sight
	if (!ctx.map->has_los(from, to))
	{
		return false;
	}

	// Check if there's an enemy at the target
	auto entity = ctx.map->get_actor(to, ctx);
	if (!entity)
	{
		return false;
	}

	// Make sure it's not the player
	if (entity == ctx.player)
	{
		return false;
	}

	return true;
}

// Process a ranged attack
bool TargetingSystem::process_ranged_attack(GameContext& ctx, Creature& attacker, Vector2D targetPos) const
{
	auto target = ctx.map->get_actor(targetPos, ctx);
	if (!target || target == &attacker)
	{
		return false;
	}

	// Check range and line of sight
	if (!is_valid_target(ctx, attacker.position, targetPos, 4))
	{
		return false;
	}

	// Get projectile info from the player
	char projectileSymbol = '*';
	int projectileColor = 2;

	// Animate the projectile
	animate_projectile(ctx, attacker.position, targetPos, projectileSymbol, projectileColor);

	// Perform the attack
	attacker.attacker->attack(attacker, *target, ctx);

	return true;
}

// Animate a projectile flying from source to target
void TargetingSystem::animate_projectile(
	GameContext& ctx,
	Vector2D from,
	Vector2D to,
	char projectileSymbol,
	int colorPair
) const
{
	if (!ctx.renderer || !ctx.rendering_manager) return;

	auto path = Map::bresenham_line(from, to);
	if (path.size() < 2) return;

	int   ts   = ctx.renderer->get_tile_size();
	int   half = ts / 2;
	float r    = static_cast<float>(ts) / 4.0f;

	static constexpr double STEP_DURATION = 0.05; // 50 ms per tile

	for (int i = 1; i < static_cast<int>(path.size()); ++i)
	{
		const auto& pos = path[i];

		double start = GetTime();
		while (GetTime() - start < STEP_DURATION && !WindowShouldClose())
		{
			int sx = pos.x * ts - ctx.renderer->get_camera_x() + half;
			int sy = pos.y * ts - ctx.renderer->get_camera_y() + half;

			ctx.renderer->begin_frame();
			ctx.rendering_manager->render(ctx);
			DrawCircle(sx, sy, r,         Color{255, 220,  50, 230}); // yellow shell
			DrawCircle(sx, sy, r * 0.5f,  Color{255, 255, 255, 220}); // white core
			ctx.renderer->end_frame();
		}
	}
}

void TargetingSystem::draw_aoe_preview(GameContext& ctx, Vector2D center, int radius) const
{
	if (!ctx.renderer || !ctx.map || radius <= 0) return;

	int ts    = ctx.renderer->get_tile_size();
	int cam_x = ctx.renderer->get_camera_x();
	int cam_y = ctx.renderer->get_camera_y();

	for (int dy = -radius; dy <= radius; ++dy)
	{
		for (int dx = -radius; dx <= radius; ++dx)
		{
			Vector2D pos{ center.x + dx, center.y + dy };
			if (!ctx.map->is_in_bounds(pos)) continue;
			if (pos.distance_to(center) > static_cast<double>(radius)) continue;

			int sx = pos.x * ts - cam_x;
			int sy = pos.y * ts - cam_y;
			DrawRectangle(sx, sy, ts, ts, Color{255, 140, 0, 50}); // orange AoE tint
		}
	}
}

bool TargetingSystem::pick_tile(GameContext& ctx, Vector2D* position, int maxRange) const
{
	return run_targeting_loop(ctx, position, maxRange, 0);
}

bool TargetingSystem::pick_tile_aoe(
	GameContext& ctx,
	Vector2D* position,
	int maxRange,
	int aoe_radius
) const
{
	return run_targeting_loop(ctx, position, maxRange, aoe_radius);
}

bool TargetingSystem::run_targeting_loop(
	GameContext& ctx,
	Vector2D* position,
	int maxRange,
	int aoe_radius
) const
{
	if (!ctx.renderer || !ctx.rendering_manager || !ctx.input_system || !ctx.map || !ctx.player)
		return false;

	Vector2D cursor = ctx.player->position;
	bool picking    = true;
	bool confirmed  = false;

	while (picking && !WindowShouldClose())
	{
		ctx.renderer->begin_frame();
		ctx.rendering_manager->render(ctx);

		draw_range_indicator(ctx, ctx.player->position, maxRange);
		draw_los(ctx, cursor);
		if (aoe_radius > 0) draw_aoe_preview(ctx, cursor, aoe_radius);

		// Pulsing cursor highlight
		{
			int   ts      = ctx.renderer->get_tile_size();
			int   sx      = cursor.x * ts - ctx.renderer->get_camera_x();
			int   sy      = cursor.y * ts - ctx.renderer->get_camera_y();
			float pulse   = (std::sin(static_cast<float>(GetTime()) * 6.28318f * 4.0f) + 1.0f) * 0.5f;
			auto  fill_a  = static_cast<unsigned char>(40.0f  + 30.0f  * pulse);
			auto  ring_a  = static_cast<unsigned char>(120.0f + 100.0f * pulse);

			DrawRectangle(sx, sy, ts, ts, Color{255, 255, 50, fill_a});
			DrawRectangleLinesEx(
				Rectangle{
					static_cast<float>(sx), static_cast<float>(sy),
					static_cast<float>(ts), static_cast<float>(ts)
				},
				2.0f,
				Color{255, 255, 50, ring_a}
			);
		}

		ctx.renderer->draw_text(4, 4, "Select target -- arrows/WASD: move  Enter: confirm  Esc: cancel", WHITE_BLACK_PAIR);
		ctx.renderer->end_frame();

		ctx.input_system->poll();
		GameKey key = ctx.input_system->get_key();

		Vector2D move{ 0, 0 };
		switch (key)
		{
		case GameKey::UP:    move = DIR_N;  break;
		case GameKey::W:     move = DIR_N;  break;
		case GameKey::DOWN:  move = DIR_S;  break;
		case GameKey::S:     move = DIR_S;  break;
		case GameKey::LEFT:  move = DIR_W;  break;
		case GameKey::A:     move = DIR_W;  break;
		case GameKey::RIGHT: move = DIR_E;  break;
		case GameKey::D:     move = DIR_E;  break;
		case GameKey::Q:     move = DIR_NW; break;
		case GameKey::E:     move = DIR_NE; break;
		case GameKey::Z:     move = DIR_SW; break;
		case GameKey::C:     move = DIR_SE; break;
		case GameKey::ENTER:  confirmed = true;  picking = false; break;
		case GameKey::ESCAPE: picking = false;                     break;
		default: break;
		}

		if (move.x != 0 || move.y != 0)
		{
			Vector2D next      = cursor + move;
			bool in_bounds     = ctx.map->is_in_bounds(next);
			bool in_range      = maxRange <= 0
				|| next.distance_to(ctx.player->position) <= static_cast<double>(maxRange);
			if (in_bounds && in_range)
				cursor = next;
		}
	}

	if (confirmed && ctx.map->is_in_bounds(cursor))
	{
		*position = cursor;
		return true;
	}
	return false;
}

// Handle ranged attack coordination
void TargetingSystem::handle_ranged_attack(GameContext& ctx) const
{
	// Get equipped missile weapon to determine range
	Item* missileWeapon = ctx.player->get_equipped_item(EquipmentSlot::MISSILE_WEAPON);
	const int weaponRange = get_weapon_range(missileWeapon);

	// Enter targeting mode with weapon-specific range
	Vector2D targetPos = select_target(ctx, ctx.player->position, weaponRange);

	// If a valid target was selected and we're in attack mode
	if (targetPos.x != -1 && targetPos.y != -1)
	{
		// Process the ranged attack (including projectile animation)
		process_ranged_attack(ctx, *ctx.player, targetPos);

		// Clean up dead creatures after ranged combat
		ctx.creature_manager->cleanup_dead_creatures(*ctx.creatures);
	}
}

// Get weapon range based on weapon type (AD&D 2e ranges)
int TargetingSystem::get_weapon_range(const Item* weapon)
{
	if (!weapon)
		return 4; // Default fallback range

	switch (weapon->itemClass)
	{
		case ItemClass::BOW:
			if (weapon->itemId == ItemId::LONG_BOW)
				return 7; // AD&D 2e: 70 yards (~7 dungeon tiles)
			else
				return 5; // AD&D 2e: 50 yards (~5 dungeon tiles) for short bow
		case ItemClass::CROSSBOW:
			return 6; // AD&D 2e: 60 yards (~6 dungeon tiles)
		default:
			return 4; // Default for unknown weapons
	}
}

TargetResult TargetingSystem::acquire_targets(GameContext& ctx, TargetMode mode, Vector2D origin, int range, int aoe_radius) const
{
    switch (mode)
    {
    case TargetMode::AUTO_NEAREST:  return target_auto_nearest(ctx, origin, range);
    case TargetMode::PICK_TILE_SINGLE: return target_pick_single(ctx);
    case TargetMode::PICK_TILE_AOE:    return target_pick_aoe(ctx, aoe_radius);
    }
    return {};
}

TargetResult TargetingSystem::target_auto_nearest(GameContext& ctx, Vector2D origin, int range) const
{
    const auto& monster = ctx.creature_manager->get_closest_monster(*ctx.creatures, origin, range);
    if (!monster)
        return {};
    return {true, monster->position, {monster}};
}

TargetResult TargetingSystem::target_pick_single(GameContext& ctx) const
{
    Vector2D pos{0, 0};
    if (!pick_tile(ctx, &pos, 0))
        return {};
    Creature* actor = ctx.map->get_actor(pos, ctx);
    if (!actor)
        return {};
    return {true, pos, {actor}};
}

TargetResult TargetingSystem::target_pick_aoe(GameContext& ctx, int aoe_radius) const
{
    Vector2D center{0, 0};
    if (!pick_tile_aoe(ctx, &center, aoe_radius, aoe_radius))
        return {};
    std::vector<Creature*> targets;
    for (const auto& c : *ctx.creatures)
    {
        if (c && !c->destructible->is_dead() && c->get_tile_distance(center) <= aoe_radius)
            targets.push_back(c.get());
    }
    return {true, center, targets};
}
